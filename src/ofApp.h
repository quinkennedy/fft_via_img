#pragma once

#include "ofMain.h"
#include "ofxFft.h"

enum {SINE, MIC, AUDIO_FILE, NOISE};

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
        void gotMessage(ofMessage msg);
        void audioReceived(float* input, int bufferSize, int nChannels);
    
    private:
    string static getAudioFilename(string s){
        return "media/audio recordings/clips/amplified/" + s;
    }
        void loadShaders();
    
        int plotHeight, bufferSize;
        
        ofxFft* fft;
        
        ofMutex soundMutex;
        vector<float> drawBins, middleBins, audioBins;
        vector<float> drawBuffer, middleBuffer, audioBuffer;
        static const int numPics = 4;
        ofImage pics[numPics];
        ofShader      shader;
        ofSoundPlayer audio;
        bool useShader;
        int picIndex;
        int mode;
        vector<string> audioFiles;
        int audioIndex;
        int appWidth, appHeight;
};

