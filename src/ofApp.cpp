#include "ofApp.h"
#include "ofApp_loadData.impl"
#include "CsoundOp.h"

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
    points.setMode(OF_PRIMITIVE_POINTS);
    prmLine.setMode(OF_PRIMITIVE_LINES);
    loadData("musical_cylinder_04_strm_01.csv");
    
    res = 10;
    numOcs = poly.size() / res;
    
    renderDuration = 30; // se
    setupCsound();
    setupOrc();
    setupSco();
    startCsound();
    
    ofExit();
}

void ofApp::setupCsound(){
    
    csound = new Csound();
    
    string renderFilePath = ofToDataPath(ofGetTimestampString() + ".wav", true);
    string rnd = "-o" + renderFilePath;
    csound->SetOption( const_cast<char*>( rnd.c_str() ) );
    csound->SetOption( (char*)"-W" );   // Wav
    csound->SetOption( (char*)"-f" );   // -f:32float, -3:24bit, -s:16bit,
}

void ofApp::setupOrc(){
    

    cout << "making Orchestra file for " << numOcs << " Sine waves" << endl;
    
    sr = 48000;    // sampling rate
    ksmps = 16;    // control rate
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

    string instr1_kfreq;
    for( int i=0; i<numOcs; i++){
        int id = i+1;
        string kfreq = mt::op_orc("kfreq"+to_string(id), "chnget", "\"cfreq"+ to_string(id)+"\"");
        instr1_kfreq += kfreq;
    }
    instr1_kfreq += "\n";
    
    string instr1_aOsc;
    
    for(int i=0; i<numOcs; i++){
        int id = i+1;
//        int octave = floor(i/8);
//        double per = (float)i/numOcs;
//        double rate = 0.2 * 0.25;
//        double freq;
//        
//        if(i%4==0){
//            freq = pow((double)2, (double)(i-numOcs)*rate) * 22000.0 + 2;
//        }else if(i%4==1){
//            freq = pow((double)7, (double)(i-numOcs)*rate) * 22000.0 + 2;
//        }else if(i%4==2){
//            freq = pow((double)15, (double)(i-numOcs)*rate) * 22000.0 + 2;
//        }else if(i%4==3){
//            freq = pow((double)23, (double)(i-numOcs)*rate) * 22000.0 + 2;
//        }
//        string freqs = ofToString(freq, 20);
        string freqs = "kfreq"+to_string(id);
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
    
    string orc = settings + gen + instr1_top + instr1_kamp + instr1_kfreq + instr1_aOsc + instr1_mix + instr1_out;
    
    int r1 = csound->CompileOrc( orc.c_str() );
    saveString( orc, ofToDataPath("orc_src.txt", true) );
    
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
    MYFLT *cfreq[numOcs];
    
    vector<double> prevAmp, prevFreq;
    prevAmp.assign(numOcs, 0);
    prevFreq.assign(numOcs, 0);
    
    double percent = 0;
    
    bool init = false;
    int totalKFrame = sr * renderDuration / ksmps;
    int currentFrame = 0;
    
    double maxLen = -1;
    for(int i=0; i<poly.size(); i++){
        float len = poly[i].getPerimeter();
        maxLen = MAX(maxLen, len);
    }
    double stepLen = maxLen / totalKFrame;
    double currentLen = 0;
    
    while ( csoundPerformKsmps(csound->GetCsound() ) == 0) {
    
        percent = (double)currentFrame/totalKFrame;
        currentFrame++;
        printf("writing %0.8f\n", percent*100.0);
        
        for( int f=0; f<numOcs; f++ ){
            string amp = "camp" + to_string(f+1);
            csoundGetChannelPtr( csound->GetCsound(), &camp[f], amp.c_str(), CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL );

            string freq = "cfreq" + to_string(f+1);
            csoundGetChannelPtr( csound->GetCsound(), &cfreq[f], freq.c_str(), CSOUND_INPUT_CHANNEL | CSOUND_CONTROL_CHANNEL );

            //glm::vec3 m = magnitude[f*res].getPointAtPercent(percent);
            
            double damp, dfreq;
            if(currentLen>=poly[f*res].getPerimeter()){
                damp =0;
                dfreq = 0;
            }else{
                glm::vec3 p = poly[f*res].getPointAtLength(currentLen);

                //double thisPercent = currentLen / poly[f*res].getPerimeter();
                //glm::vec3 m = magnitude[f*res].getPointAtPercent(thisPercent);
                
                glm::vec2 yaxis(0,1);
                glm::vec2 p2(p.x, p.y);
                double length = glm::length(p2);
                glm::vec2 p2n = glm::normalize(p2);
                double angle = glm::orientedAngle(yaxis, p2n);
                float angleRate = ofMap(angle, -TWO_PI, TWO_PI, 0, 100);
                dfreq = pow(2, angleRate*0.2) + 20;
                damp = length * 0.001;
            }
            
            if(init){
                damp  = damp *0.0001 + prevAmp[f]*0.9999;
                dfreq = dfreq*0.0001 + prevFreq[f]*0.9999;
            }
            *camp[f] = damp;
            *cfreq[f] = dfreq;
            prevAmp[f] = damp;
            prevFreq[f] = dfreq;
        }
        
        currentLen += stepLen;
        init = true;
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
        
        glm::vec3 v = p.getPointAtPercent(percent);
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
    
    // sender
    // Midi Ch      : 1 ~ 16
    int maxMidiCh = 16;
    int dataWidth = numPoly/maxMidiCh;

    vector<vector<int>> prms(maxMidiCh, vector<int>(10));

    // check intersect & send noteOn
    for(int i=0; i<maxMidiCh; i++){
        
        vector<int> & prm = prms[i];
        
        int lineId = i * dataWidth;

        // Note On
        ofPolyline & p = poly[lineId];
        ofPolyline & m = magnitude[lineId];
        glm::vec3 v = p.getPointAtPercent(percent);
        glm::vec3 mag = m.getPointAtPercent(percent);
        
        prm[0] = 1;
        
        for(int j=0; j<prm.size(); j++){
            int prmId = j+1;
            
            glm::vec3 axis(0,1,0);
            glm::vec3 vn = glm::normalize(v);
            float angle = glm::angle(vn, axis);
             float val = ofMap(angle, -PI, PI, 0, 127);
            prm[prmId] = val;
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
