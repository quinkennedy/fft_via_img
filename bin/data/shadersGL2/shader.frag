#version 120

uniform float u_time;
uniform vec2 u_resolution;
uniform vec2 u_mouse;
uniform sampler2DRect tex0;
uniform float u_fft[257];

varying vec2 texCoordVarying;

vec3 HUEtoRGB(in float H)
{
  float R = abs(H * 6 - 3) - 1;
  float G = 2 - abs(H * 6 - 2);
  float B = 2 - abs(H * 6 - 4);
  return clamp(vec3(R,G,B), 0., 1.);
}

float Epsilon = 1e-10;

vec3 RGBtoHCV(in vec3 RGB)
{
  // Based on work by Sam Hocevar and Emil Persson
  vec4 P = (RGB.g < RGB.b) ? vec4(RGB.bg, -1.0, 2.0/3.0) : vec4(RGB.gb, 0.0, -1.0/3.0);
  vec4 Q = (RGB.r < P.x) ? vec4(P.xyw, RGB.r) : vec4(RGB.r, P.yzx);
  float C = Q.x - min(Q.w, Q.y);
  float H = abs((Q.w - Q.y) / (6 * C + Epsilon) + Q.z);
  return vec3(H, C, Q.x);
}

vec3 RGBtoHSL(in vec3 RGB)
{
  vec3 HCV = RGBtoHCV(RGB);
  float L = HCV.z - HCV.y * 0.5;
  float S = HCV.y / (1 - abs(L * 2 - 1) + Epsilon);
  return vec3(HCV.x, S, L);
}

vec3 HSLtoRGB(in vec3 HSL)
{
  vec3 RGB = HUEtoRGB(HSL.x);
  float C = (1 - abs(2 * HSL.z - 1)) * HSL.y;
  return (RGB - 0.5) * C + HSL.z;
}

vec3 RGBtoHSV(in vec3 RGB)
{
  vec3 HCV = RGBtoHCV(RGB);
  float S = HCV.y / (HCV.z + Epsilon);
  return vec3(HCV.x, S, HCV.z);
}

vec3 HSVtoRGB(in vec3 HSV)
{
  vec3 RGB = HUEtoRGB(HSV.x);
  return ((RGB - 1) * HSV.y + 1) * HSV.z;
}

float gaussian(float x, float amplitude, float center, float deviation)
{
  float upper = -((x - center) * (x - center));
  float lower = 2 * deviation * deviation;
  return (amplitude * exp(upper / lower));
}

float getAmpAtGaussImpl(vec3 hsv, float hue){
  float gaussAmp = hsv[2];
  float gaussMid = hsv[0];
  float gaussDev = 1. - hsv[1];
  gaussDev *= gaussDev;

  // this probably doesn't need to be gaussian
  //  we need to do circular math (.9 == -.1)
  //  so we need to combine two calculations for red
  //  but combining for greens & cyans over-compensates
  float combineAlpha = 0.5 - gaussian(hue, .5, .5, .13);

  // calculate actual value
  float amp = gaussian(hue, gaussAmp, gaussMid, gaussDev);

  // wrap value around nearest edge
  if (hue > .5) {
    hue -= 1.;
  } else {
    hue += 1.;
  }

  // calculate secondary value to account for circular hue
  float amp2 = gaussian(hue, gaussAmp, gaussMid, gaussDev);
  // mix the calculations together
  amp = mix(amp, amp2, combineAlpha);
  return amp;
}

float getAmpAtCenterImpl(vec3 hsv, float hue){
  float gaussAmp = hsv[2];
  float gaussMid = hsv[0];
  float gaussDev = 1. - (hsv[1]);
  gaussDev *= gaussDev;

  // rotate gaussMid and hue so hue <= .5
  gaussMid += (.5 - hue);
  hue = .5;
  // wrap gaussMid to stay in [0,1]
  if (gaussMid > 1){
    gaussMid -= 1;
  } else if (gaussMid < 0){
    gaussMid += 1;
  }

  return gaussian(hue, gaussAmp, gaussMid, gaussDev);
}

float getAmpAt(vec3 hsv, float hue){
  //return getAmpAtGaussImpl(hsv, hue);
  return getAmpAtCenterImpl(hsv, hue);
}

void showRotatingColor(){
  vec4 imgColor = texture2DRect(tex0, texCoordVarying);
  float highlightHue = fract(u_time / 4.);
  vec3 hsv = RGBtoHSV(imgColor.rgb);
  float amplitude = getAmpAt(hsv, highlightHue);
  vec3 rgb = HSVtoRGB(vec3(hsv[0], 1., hsv[2]));
  gl_FragColor = vec4(rgb, 1.0);
}

void showSumHueHighlight(){
  // grab the color from the texture
  vec4 imgColor = texture2DRect(tex0, texCoordVarying);
  vec3 hsv = RGBtoHSV(imgColor.rgb);

  float highlightHue = 0;
  float amplitude = 0;
  vec3 rgb = vec3(0, 0, 0);

  int numSections = 6;//u_fft.length();
  for(int i = numSections; i >= 0; i--){
    highlightHue = (float(i) / float(numSections));
    amplitude = getAmpAt(hsv, highlightHue);
    rgb += HSVtoRGB(vec3(highlightHue, 1., amplitude )) / float(numSections / 2);
  }
  gl_FragColor = vec4(rgb, 1.0);
}

void showRawFFT(){
  int myFreq = int(floor((gl_FragCoord.x / u_resolution.x) * u_fft.length()));
  float fftVal = u_fft[myFreq];
  gl_FragColor = vec4(fftVal, fftVal, fftVal, 1);
}

void showGaussianFunc(){
  float x = (gl_FragCoord.x / u_resolution.x);
  float fx = gaussian(x, u_mouse.y / u_resolution.y, .5, .005);//u_mouse.x / u_resolution.x);
  float y = (gl_FragCoord.y / u_resolution.y);
  float yu = ((gl_FragCoord.y + 1) / u_resolution.y);
  float yd = ((gl_FragCoord.y - 1) / u_resolution.y);
  if (abs(y - fx) < abs(yu - fx) && abs(y - fx) <= abs(yd - fx)){
    gl_FragColor = vec4(1, 1, 1, 1);
  } else {
    //gl_FragColor = vec4(0, 0, 0, 1);
  }
}

void main()
{
  //showRawFFT();
  //showGaussianFunc();
  //return;
  //showSumHueHighlight();
  //return;
  // grab the color from the texture
  vec4 imgColor = texture2DRect(tex0, texCoordVarying);

  int myFreq = int(floor((gl_FragCoord.x / u_resolution.x) * u_fft.length()));
  float fftVal = u_fft[myFreq];

  // highlight a hue based on mouse position
  //float highlightHue = clamp(u_mouse.x / u_resolution.x, 0., 1.);
  //float highlightHue = fract(u_time / 4.);
  float highlightHue = 0;//(float(myFreq) / float(u_fft.length()));

  // first covert to HSV
  //  Hue: encoding the central wavelength of the color
  //  Saturation: how "tight" to the wavelength the color is
  //  Value: ~brightness.
  // The key differentiator to HSL, 
  //  is that V@1.0 allows for maximum saturation
  //  however L@1.0 is always white
  // Essentially:
  //  V sets the maximum amplitude
  //  S is inverse to the [Q factor](https://en.wikipedia.org/wiki/Q_factor)
  //  H is the center of the frequency band
  vec3 hsv = RGBtoHSV(imgColor.rgb);
  vec3 rgb = vec3(0., 0., 0.);
  float amplitude = 0.;

  for(int i = (u_fft.length() - 1); i >= 0; i--){
    highlightHue = (float(i) / float(u_fft.length()));
    amplitude = getAmpAt(hsv, highlightHue);
    // attenuate based on audio level
    float attenuation = sqrt(u_fft[i]);//max(u_fft[i], log(u_fft[i]) + 1);
    //attenuation *= attenuation;
    amplitude *= attenuation;//(1. - attenuation);
    rgb += HSVtoRGB(vec3(highlightHue, 1., amplitude)) / float(u_fft.length() / 2);
  }

  // attenuate based on mouse position
  //amplitude *= (u_mouse.y  / u_resolution.y);

  //vec3 rgb = HSVtoRGB(vec3(highlightHue, 1., amplitude));
  //vec3 rgb = HSVtoRGB(vec3(hsv[0], 1., hsv[2]));

  gl_FragColor = vec4(rgb, 1.0);
}
