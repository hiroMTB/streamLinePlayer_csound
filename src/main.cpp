//#include "JuceLibraryCode/JuceHeader.h"
#include "ofMain.h"
#include "ofApp.h"

int main(int argc, char* argv[]){
	ofSetupOpenGL(1920/2,1080,OF_WINDOW);
        
    if( argc == 2 ) {
        printf("FreqMode is %s\n", argv[1]);
        ofApp::freqMode = ofToInt(argv[1]);
    }else if(argc == 3){
        printf("FreqMode is %s\n", argv[1]);
        printf("RenderDuration is %s\n", argv[2]);

        ofApp::freqMode = ofToInt(argv[1]);
        ofApp::renderDuration = ofToInt(argv[2]);
    }

    ofApp * app = ofApp::get();
    
    ofRunApp(ofApp::get());
}
