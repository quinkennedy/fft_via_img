#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetLogLevel(OF_LOG_NOTICE);
    ofLogVerbose() << "starting";
    loadShaders();
    useShader = true;
    
    plotHeight = 128;
    bufferSize = 512;
    
    fft = ofxFft::create(bufferSize, OF_FFT_WINDOW_HAMMING);
    // To use FFTW, try:
    //fft = ofxFft::create(bufferSize, OF_FFT_WINDOW_HAMMING, OF_FFT_FFTW);
    
    drawBuffer.resize(bufferSize);
    middleBuffer.resize(bufferSize);
    audioBuffer.resize(bufferSize);
    
    ofLogNotice() << fft->getBinSize() << " fft bins";
    drawBins.resize(fft->getBinSize());
    middleBins.resize(fft->getBinSize());
    audioBins.resize(fft->getBinSize());
    
    // 0 output channels,
    // 1 input channel
    // 44100 samples per second
    // [bins] samples per buffer
    // 4 num buffers (latency)
    
    ofSoundStreamSetup(0, 1, this, 44100, bufferSize, 4);
    
    mode = AUDIO_FILE;
    appWidth = ofGetWidth();
    appHeight = ofGetHeight();
    
    audioFiles = vector<string>({
        "deer clearing pre-dawn.wav",
        "deer clearing pre-dawn distant bird.wav",
        "deer clearing pre-dawn early bird.wav",
        "deer clearing first light 1.wav",
        "deer clearing first light 2.wav",
        "deer clearing first light decline.wav"
    });
    audioIndex = 0;
    audio.load(getAudioFilename(audioFiles[audioIndex]));
    ofLogNotice() << "playing " << getAudioFilename(audioFiles[audioIndex]);
    audio.setVolume(1);
    audio.setLoop(true);
    audio.play();
    
    ofBackground(0, 0, 0);
    
    pics[0].load("media/pictures/IMG_0062.png");
    pics[1].load("all_colors.png");
    pics[2].load("Colouring_pencils.jpeg");
    pics[3].load("stained_glass.jpeg");
    picIndex = 0;
}

void ofApp::loadShaders(){
    ofLogNotice() << "loading shaders";
#ifdef TARGET_OPENGLES
    shader.load("shadersES2/shader");
#else
    if(ofIsGLProgrammableRenderer()){
        shader.load("shadersGL3/shader");
    }else{
        shader.load("shadersGL2/shader");
    }
#endif
}

//--------------------------------------------------------------
void ofApp::update(){
    ofSoundUpdate();
    if (mode == AUDIO_FILE){
        float * val = ofSoundGetSpectrum(fft->getBinSize());
        for(int i = 0; i < fft->getBinSize(); i++){
            middleBins[i] = MAX(middleBins[i] * .95, val[i]);
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    //ofBackground(0, 0, 0);
    //maskFbo.begin();
    if (useShader){
        shader.begin();
        shader.setUniform1f("u_time", ofGetElapsedTimef());
        shader.setUniform2f("u_resolution", ofGetWidth(), ofGetHeight());
        shader.setUniform2f("u_mouse", ofGetMouseX(), ofGetMouseY());
        soundMutex.lock();
        shader.setUniform1fv("u_fft", middleBins.data(), fft->getBinSize());
        soundMutex.unlock();
    }
    //ofRect(0, 0, maskFbo.getWidth(), maskFbo.getHeight());
    pics[picIndex].draw(0, 0, ofGetWidth(), ofGetHeight());
    //ofRect(0, 0, ofGetWidth(), ofGetHeight());
    if (useShader){
        shader.end();
    }
    //maskFbo.end();
}

void ofApp::audioReceived(float* input, int bufferSize, int nChannels) {
    if (mode == MIC) {
        // store input in audioInput buffer
        memcpy(&audioBuffer[0], input, sizeof(float) * bufferSize);
        
        float maxValue = 0;
        for(int i = 0; i < bufferSize; i++) {
            if(abs(audioBuffer[i]) > maxValue) {
                maxValue = abs(audioBuffer[i]);
            }
        }
        for(int i = 0; i < bufferSize; i++) {
            audioBuffer[i] /= maxValue;
        }
        
    } else if (mode == NOISE) {
        for (int i = 0; i < bufferSize; i++)
            audioBuffer[i] = ofRandom(-1, 1);
    } else if (mode == SINE) {
        for (int i = 0; i < bufferSize; i++)
            audioBuffer[i] = sinf(PI * i * mouseX / appWidth);
    }
    
    if (mode != AUDIO_FILE){
    
    fft->setSignal(&audioBuffer[0]);
    
    float* curFft = fft->getAmplitude();
    memcpy(&audioBins[0], curFft, sizeof(float) * fft->getBinSize());
    
//    //scale to loudest sound
//    float maxAmp = 0;
//    for(int i = 0; i < fft->getBinSize(); i++) {
//        if(abs(audioBins[i]) > maxAmp) {
//            maxAmp = abs(audioBins[i]);
//        }
//    }
//    for(int i = 0; i < fft->getBinSize(); i++) {
//        audioBins[i] /= maxAmp;
//    }
    
//    int spectrogramWidth = (int) spectrogram.getWidth();
//    int n = (int) spectrogram.getHeight();
//    
//    for(int i = 0; i < n; i++) {
//        int j = (n - i - 1) * spectrogramWidth + spectrogramOffset;
//        int logi = ofMap(powFreq(i), powFreq(0), powFreq(n), 0, n);
//        spectrogram.setColor(j, (unsigned char) (255. * audioBins[logi]));
//    }
//    spectrogramOffset = (spectrogramOffset + 1) % spectrogramWidth;
    
    soundMutex.lock();
    middleBuffer = audioBuffer;
    middleBins = audioBins;
    soundMutex.unlock();
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if (key == 'l' || key == 'L'){
        loadShaders();
    } else if (key == 't' || key == 'T'){
        ofLogNotice() << "toggling shader";
        useShader = !useShader;
    } else if (key == 'p' || key == 'P'){
        picIndex = ((picIndex + 1) % numPics);
    } else if (key == 'm' || key == 'M'){
        mode = (mode + 1) % ((int)NOISE + 1);
        if (mode != AUDIO_FILE){
            audio.stop();
        } else {
            audio.play();
        }
        ofLogNotice() << "mode " << mode;
    } else if (key == 'a' || key == 'A'){
        audioIndex = (audioIndex + 1) % audioFiles.size();
        ofLogNotice() << "loading audio " << audioFiles[audioIndex];
        audio.load(getAudioFilename(audioFiles[audioIndex]));
        if (mode == AUDIO_FILE){
            audio.play();
        }
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
