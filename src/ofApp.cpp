#include "ofApp.h"
#include "ofApp_loadData.impl"
#include "CsoundOp.h"

int ofApp::freqMode = 1;
float ofApp::renderDuration = 30;

void ofApp::setup(){
    
    ofSetLogLevel(OF_LOG_VERBOSE);
    ofSetWindowPosition(0,0);
    
    ofEnableAlphaBlending();
    ofSetCircleResolution(60);
    ofSetFrameRate(30);

    // camera settings
    cam.setNearClip(1);
    cam.setFarClip(10000);
    
    // csv data load
    mesh.push_back(ofVboMesh());
    mesh.back().setMode(OF_PRIMITIVE_LINE_STRIP);
    poly.push_back(ofPolyline());
    magnitude.push_back(ofPolyline());
    rotation.push_back(ofPolyline());

    points.setMode(OF_PRIMITIVE_POINTS);
    prmLine.setMode(OF_PRIMITIVE_LINES);
    
    for(int i=0; i<6;i++){
        loadData("csv/final_6ch/c" +ofToString(i+1) + ".csv");
    }

    maxLen = -1;
    for(int i=0; i<poly.size(); i++){
        float len = poly[i].getPerimeter();
        maxLen = MAX(maxLen, len);
    }
    
    freqRange = {
        ofRange(  10,   80),
        ofRange( 80,   640),
        ofRange( 640,  1280),
        ofRange(1280,  2560),
        ofRange(2560,  5120),
        ofRange(5120, 20480)
    };
    
    sr = 48000*2;   // sampling rate
    ksmps = 16;     // control rate
    res = 2;
    numOcs = poly.size() / res;
    
//    setupCsound();
//    setupOrc();
//    setupSco();
//    startCsound();
//    ofExit();
}

void ofApp::setupCsound(){
    
    csound = new Csound();
    
    //string renderFilePath = ofToDataPath(ofGetTimestampString() + ".wav", true);
    string renderFilePath = ofToDataPath("strm_"+ofToString(freqMode) + ".wav", true);
    string rnd = "-o" + renderFilePath;
    csound->SetOption( const_cast<char*>( rnd.c_str() ) );
    csound->SetOption( (char*)"-W" );   // Wav
    csound->SetOption( (char*)"-f" );   // -f:32float, -3:24bit, -s:16bit,
}

void ofApp::setupOrc(){
    

    cout << "making Orchestra file for " << numOcs << " Sine waves" << endl;
    

    int nChn = 2;
    int zdbfs = 1;
    string settings = mt::op_setting(sr, ksmps, nChn,zdbfs);
    
    
    string gen = R"dlm(
    
    giSine    ftgen     0, 0, 2^16, 10, 1
    
    )dlm";
    
    string instr1_top = R"dlm(
    
    instr 1

    )dlm";
    
    string instr1_kamp;
    for( int i=0; i<numOcs; i++){
        int id = i+1;
        string kamp = mt::op_orc("kamp"+to_string(id), "chnget", "\"camp"+ to_string(id)+"\"");
        instr1_kamp += kamp;
    }
    
    instr1_kamp += "\n";
    
    string instr1_aOsc;
    
    for(int i=0; i<numOcs; i++){
        int id = i+1;
        int octave = floor(i/8);
        double per = (float)i/numOcs;
        double rate = 0.03;
        double freq;
        
        freq = pow((double)2, (double)(i-numOcs)*rate) * freqRange[freqMode-1].max + freqRange[freqMode-1].min;
        
        string freqs = ofToString(freq, 20);
        string aOsc = mt::op_orc("aOsc"+to_string(id), "poscil", "kamp"+to_string(id), freqs, "giSine" );
        instr1_aOsc += aOsc;
    }
    
    instr1_aOsc += "\n";
    
    string instr1_mix;
    string aL = "aL  sum ";
    string aR = "aR  sum ";
    
    for( int i=0; i<numOcs; i++ ){
        int id = i+1;
        if( id%2==1)
            aL += "aOsc" + to_string(id) + ",";
        else
            aR += "aOsc" + to_string(id) + ",";
    }
    
    aL.erase(aL.size()-1);
    aR.erase(aR.size()-1);
    aL += "\n";
    aR += "\n";
    instr1_mix = aL + aR;
    
    string instr1_out = R"dlm(
    
    outs   aL, aR
    endin
    
    )dlm";
    
    string orc = settings + gen + instr1_top + instr1_kamp + instr1_aOsc + instr1_mix + instr1_out;
    
    
    int r1 = csound->CompileOrc( orc.c_str() );
    saveString( orc, ofToDataPath("orc_src_"+ofToString(freqMode)+".txt", true) );
    
    if( r1 ==0 ){
        cout << "Orcestra file compile OK\n";
    }else{
        cout << "Orcestra file compile Failed\n";
        exit();
    }
}

void ofApp::setupSco(){
    
    string sco = "i 1 0 " + ofToString(renderDuration) + "\n";
    
    cout << sco << endl;
    int r2 = csound->ReadScore(sco.c_str());
    if( r2 ==0 ){
        cout << "Score file compile OK\n";
    }else{
        cout << "Score file compile Failed\n";
    }
}

void ofApp::startCsound(){
    
    csound->Start();
    
    int loop = 0;
    
    MYFLT *camp[numOcs];
    
    vector<double> prevAmp;
    
    prevAmp.assign(numOcs, 0);
    
    double percent = 0;
    
    int totalKFrame = sr * renderDuration / ksmps;
    int currentFrame = 0;
    
    
    double stepLen = maxLen / totalKFrame;
    double currentLen = 0;
    
    while ( csoundPerformKsmps(csound->GetCsound() ) == 0) {
    
        percent = (double)currentFrame/totalKFrame;
        currentFrame++;
        printf("writing %0.8f\n", percent*100.0);
        
        for( int f=0; f<numOcs; f++ ){
            string chname = "camp" + to_string(f+1);
            csoundGetChannelPtr( csound->GetCsound(), &camp[f], chname.c_str(), CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL );
            
            //glm::vec3 m = magnitude[f*res].getPointAtPercent(percent);
            
            double amp;
            if(currentLen>=poly[f*res].getPerimeter()){
                amp =0;
            }else{
                glm::vec3 p = poly[f*res].getPointAtLength(currentLen);
                double thisPercent = currentLen / poly[f*res].getPerimeter();
                glm::vec3 m = magnitude[f*res].getPointAtPercent(thisPercent);
                amp = m.y * 0.04;
            }
            
            amp = amp*0.0001 + prevAmp[f]*0.9999;
            *camp[f] = amp;
            prevAmp[f] = amp;
        }
        
        currentLen += stepLen;
    }
}

void ofApp::update(){
    
    int totalFrame = ofGetTargetFrameRate() * renderDuration;
    frame = ++frame % totalFrame;
    float percent = (float)frame/totalFrame;
    
    points.clear();
    prmLine.clear();
    

    int numPoly = poly.size();
    
    
    for( int i=0; i<numPoly; i++){
        ofPolyline & p = poly[i];
        ofPolyline & m = magnitude[i];
        ofPolyline & r = rotation[i];
        
        double currentLen = maxLen * percent;
        glm::vec3 v = p.getPointAtLength(currentLen);
        glm::vec3 mag = m.getPointAtPercent(percent);
        glm::vec3 rot = r.getPointAtPercent(percent);
        
        ofFloatColor red(1,0,0);
        ofFloatColor blue(0,0,1);
        ofFloatColor col;

        float len = rot.y * 0.004;
        col =  red*len + blue*(1.0-len);
        col.a = 0.9;
        
        points.addVertex(v);
        points.addColor(col);
        
        {
            glm::vec3 zerovec(0,0,v.z);
            prmLine.addVertex(zerovec);
            prmLine.addVertex(v);
            col.a = 0.4;
            prmLine.addColor(ofFloatColor(col));
            prmLine.addColor(ofFloatColor(col));
        }
    }
}

void ofApp::draw(){
   
    ofEnableAntiAliasing();
    ofBackground(255);
    
    cam.begin(); {
 
        if(cam.getOrtho()){
            ofDisableDepthTest();
            ofScale(3, 3);
        }else{
            ofEnableDepthTest();
        }
        
        ofDrawAxis(100);
        
        if(!bPrmMode){
            for(auto & m : mesh){
                m.draw();
            }
        }else{
            prmLine.draw();
        }
        
        // indicator
        glPointSize(3);
        points.draw();

        int totalFrame = ofGetTargetFrameRate() * renderDuration;
        float percent = (float)frame/totalFrame;
        double currentLen = maxLen * percent;
        
        for(int i=0; i<firstLineId.size(); i++){
            glm::vec3 v = poly[firstLineId[i]+600].getPointAtLength(currentLen);
            
            ofFill();
            ofSetColor(0, 255, 0);
            ofDrawSphere(v, 2);
        }

        
    }cam.end();
}

void ofApp::keyPressed(int key){
    
    switch(key){
        case 'O':
            cam.getOrtho() ? cam.disableOrtho() : cam.enableOrtho();
            break;
        
        case 'P':
            bPrmMode = !bPrmMode;
            break;
            
        case '0':
            frame = 0;
            break;
            
        default:
            break;
    }
}

void ofApp::exit(){
    if(csound) csoundDestroy( csound->GetCsound() );
}

void ofApp::saveString( string str, filesystem::path path ){
    ofstream ost( path.string() );
    ost << str;
    ost.close();
}
