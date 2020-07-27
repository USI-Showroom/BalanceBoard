import processing.serial.*;
import processing.opengl.*;
import toxi.geom.*;
import toxi.processing.*;
import http.requests.*;

int PORT_NUM = 2;

//calibration
int calSamples;
double calYaw;
double calPinch;
double calRoll;
double yaw;
double pinch;
double roll;
boolean calDone;
//Save the result on a server online
String siteName= "http://127.0.0.1/";
String textValue;
String YPRdata;
Serial port;   
float lastScore;
int secondsToGo;
float bestScore;
int centerx; //X center of the screen;
int centery; //Y center of the screen;
float score; // sum of all the points;
int samples; //number of samlpes to do an average;
float mx; //X position of the balanceboard to calculate points;
float my; //Y position of the balanceboard to calculate points;
float mz;
int numberOfCircles; //number Of Circles it determinates also the max points you can get. 
int radiusOfCentralCircle; // basically the precision between one point area and another
//numer of colors in the colorArray must be greather or equal to the number of circles.
color[] colorArray = { color(128,0,0),
                      color(255,36,0),
                      color(255,153,0),
                      color(255,255,0), 
                      color(204,255,0),
                      color(102,255,0),
                      color(220,200,85),
                      color(185,65,200),
                      color(100,255,35),
                      color(220,200,85),
                      color(185,65,200),
                      color(0,145,35), 
                      color(245,35,200),
                      color(100,255,35),
                      color(220,200,85),
                      color(185,65,200)};
                      
float multiplier;
//different states that the machine can have;

String currentState;
int countdown;
//aspects:
color fontColor = color(255,255,255);
color backgoundColor = color(0,0,0);
boolean first;
void setup(){

  calYaw=0;
  calPinch=0;
  calRoll=0;
  yaw=0;
  pinch=0;
  roll=0;
  //for username
  textValue = "";
  //
  YPRdata ="";
  score =0;
  samples = 0;
  numberOfCircles = 6;
  radiusOfCentralCircle = 50;  
  currentState = "CALIBRATION";
  lastScore=-1.0;
  bestScore=-1.0;
  multiplier = (numberOfCircles*radiusOfCentralCircle)/0.20;
  int w = numberOfCircles*radiusOfCentralCircle*2+50;
  int h = numberOfCircles*radiusOfCentralCircle*2+200;
//  size(w,h, OPENGL);
  size(650,800, OPENGL);
  background(backgoundColor);
   centerx = width/2;
  centery = height/2;
  println(Serial.list()[PORT_NUM]);
  String portName = Serial.list()[PORT_NUM];
  port = new Serial(this, portName, 115200);
  calSamples=0;
  calDone= false;
  first = true;
}
void draw(){
  noStroke();
  textSize(26);
  backgoundColor = color(0,0,0);
  pushMatrix();
        fill(backgoundColor);
        rect(0,0,width,height);
  popMatrix();
  if (currentState.equals("CALIBRATION")) {
    // Calibration state should last 20 second or so, done at the start of the program.
    pushMatrix();
      translate(width/2,height/2);
      textAlign(CENTER);
      fill(fontColor);
      text(String.format("Secondi rimasti per la calibrazione: %d", secondsToGo ) ,0,0);
    popMatrix();
  }else if (currentState.equals("SENDDATA")) {
    showScore();
    pushMatrix();
      translate(width/2,height/2-30);
      fill(fontColor);
      textAlign(CENTER);
      text("Inserire il proprio nome:",0,0);
      translate(0,60);
      fill(fontColor);
      textAlign(CENTER);
      text(textValue,0,0);
    popMatrix();
  }else if (currentState.equals("ENDING")) {
    showScore();
    pushMatrix();
      translate(width/2,height/2);
      fill(fontColor);
      textAlign(CENTER);
      text("Prova finita, scendere dalla pedana",0,0);
    popMatrix();
  }else if (currentState.equals("WAITING")) {
    showScore();
    pushMatrix();
      fill(fontColor);
      textAlign(CENTER);
      translate(width/2,height/2);
      text("Sali sulla pedana per iniziare!",0,0);
    popMatrix();  
  } else if (currentState.equals("SETTINGS")) {
    fill(fontColor);
//    pushMatrix();
//      translate(width/2,60);
//      textAlign(CENTER);
//      text("Timers :",0,0);
//    popMatrix();
//    
//    pushMatrix();
//      translate(20,30+(height*2/3-30)/3);
//      textAlign(LEFT);
//      text("Calibration: ",0,0);
//      translate(width-40,0);
//      textAlign(RIGHT);
//      text(String.format("%d s",calTimer/1000),0,0);
//    popMatrix();
//    
//    pushMatrix();
//      translate(20,((height*2/3-30)/3 )*2);
//      textAlign(LEFT);
//      text("Attempt: ",0,0);
//      translate(width-40,0);
//      textAlign(RIGHT);
//      text(String.format("%d s",attTimer/1000),0,0);
//    popMatrix();
//    
//    pushMatrix();
//      translate(20,-30+(height*2/3-30));
//      textAlign(LEFT);
//      text("Ending: ",0,0);
//      translate(width-40,0);
//      textAlign(RIGHT);
//      text(String.format("%d s",endTimer/1000),0,0);
//    popMatrix();
//    
//    pushMatrix();
//      translate(width/2,height*2/3+60);
//      textAlign(CENTER);
//      text("Site: ",0,0);
//    popMatrix();
//    
//    pushMatrix();
//      translate(width/2,height*2/3+120);
//      textAlign(CENTER);
//      text(String.format("%s",siteName),0,0);
//    popMatrix();

  }else if (currentState.equals("COUNTDOWN")) {
    textAlign(CENTER);
    pushMatrix();
      translate(width/2,height/2);
      fill(fontColor);
      if((secondsToGo)>=1){
        text(secondsToGo-1 ,0,0);
      }else{
        text("VIA!" ,0,0);
      }
    popMatrix(); 
  } else if (currentState.equals("ATTEMPT")) {
    pushMatrix();
      fill(fontColor);
      textAlign(CENTER);
      translate(width/2,height-30);
      text(String.format("Secondi rimasti: %d",secondsToGo) ,0,0);
    popMatrix();
    //the circle
    pushMatrix();
     translate(centerx, centery);
     
      for(int i=0; i<numberOfCircles;i++){
        fill(colorArray[i]);
        int dim = (radiusOfCentralCircle*2) * (numberOfCircles-i);
        ellipse(0,0,dim,dim);
      }
   popMatrix();
   //punto del baricentro
   pushMatrix();
     stroke(255);
     strokeWeight(5);
     fill(0);
     translate(width/2,height/2);
     if(sqrt((mx*mx)+(my*my))<(numberOfCircles * radiusOfCentralCircle - 5 )){
       //TODO change this with the shis
       translate(my, mx);
       ellipse(0,0,15,15);
     }
   popMatrix();
   
   //This is for the score on top;
   pushMatrix();
     textAlign(CENTER);
     translate(centerx,60);
     fill(fontColor);
     text(String.format("Punteggio: %.1f / %d",score, numberOfCircles),0,0);
   popMatrix();
  }
  
   
   
}

void keyPressed() {
  if(currentState.equals("SENDDATA")){
    if (key != CODED) {
         switch(key) {
           case BACKSPACE:
             textValue = textValue.substring(0,max(0,textValue.length()-1));
             break;
           case TAB:
             textValue += "    ";
             break;
           case ENTER:
           case RETURN:
             //here i should send the text via http
             String scoretext = String.format("%.2f",lastScore);
             GetRequest post = new GetRequest(siteName+textValue+"/"+scoretext);
             post.send();
             port.write("ok\n");
             textValue ="";
        // comment out the following two lines to disable line-breaks
        //typedText += "\n";
             break;
           case ESC:
           case DELETE:
             break;
           default:
             textValue += key;
             println(textValue);
      }
    }
  }//else{
//    if (currentState.equals("SETTINGS")) {
//      if(((key=='s' || key=='S')&&!s)||(s && !c && !a && !e)){
//        if(key==ENTER || key==RETURN){
//          //save changes 
//          s= false;
//          try{
//            String [] ping =loadStrings(siteName);
//            println(ping[0]);
//            
//          }catch(Throwable e){
//            siteName="";
//          }
//          if(siteName!=""){
//            return;
//          }
//        }
//        if(!s){
//          a = false;
//          c = false;
//          e = false;
//          s = true;
//          siteName="";
//          println("you want to change site.");
//        }else{
//          if(key==65535){ //check for shift
//            return;
//          }
//          switch(key) {
//           case BACKSPACE:
//             siteName = siteName.substring(0,max(0,siteName.length()-1));
//           case TAB:
//           case ENTER:
//           case RETURN:
//           case ESC:
//           case DELETE:
//             break;
//           default:
//             siteName += key;
//             
//             siteName = trim(siteName);
//             print(siteName);
//             println(textValue);
//          }
//        }
//        
//        //we want to change the site domain
//        
//        
//      }else if(((key=='c' || key=='C')&&!c)||(!s && c && !a && !e)){
//        if(key==ENTER || key==RETURN){
//          //save changes 
//          c= false;
//          
//          return;
//        }
//        if(!c){
//          a = false;
//          c = true;
//          e = false;
//          s = false;
//          calTimer = 0;
//          println("you want to change calibration timer.");
//        }else{
//          if(char(key)>='0' && char(key)<='9'){
//             calTimer = calTimer/1000;
//             calTimer = calTimer * 10;
//             calTimer += (char(key)-48);
//             calTimer = calTimer * 1000;
//             println(calTimer);
//          }else if(key==BACKSPACE){
//             String time = nf(calTimer/1000, 10);
//             time = time.substring(0,max(0,time.length()-1));
//             calTimer = parseInt(time);
//             calTimer=calTimer*1000;
//          }
//        }
//        
//
//        //we want to change the calibration timer
//        
//      
//      }else if(((key=='a' || key=='A')&&!a)||(!s && !c && a && !e)){
//        if(key==ENTER || key==RETURN){
//          //save changes 
//          a= false;
//          return;
//        }
//        if(!a){
//          a = true;
//          c = false;
//          e = false;
//          s = false;
//          attTimer = 0;
//          println("you want to change attempt timer.");
//        }else{
//           if(char(key)>='0' && char(key)<='9'){
//             attTimer = attTimer/1000;
//             attTimer = attTimer * 10;
//             attTimer += (char(key)-48);
//             attTimer = attTimer * 1000;
//             println(attTimer);
//          }else if(key==BACKSPACE){
//             String time = nf(attTimer/1000, 10);
//             time = time.substring(0,max(0,time.length()-1));
//             attTimer = parseInt(time);
//             attTimer=attTimer*1000;
//          }
//        }
//        //we want to change the attempt timer
//      }else if(((key=='e' || key=='E')&&!e)||(!s && !c && !a && e)){
//        if(key==ENTER || key==RETURN){
//          //save changes 
//          e= false;
//          return;
//        }
//        if(!e){
//          a = false;
//          c = false;
//          e = true;
//          s = false;
//          endTimer = 0;
//          println("you want to change ending timer.");
//        }else{
//         
//           if(char(key)>='0' && char(key)<='9'){
//             endTimer = endTimer/1000;
//             endTimer = endTimer * 10;
//             endTimer += (char(key)-48);
//             endTimer = endTimer * 1000;
//             println(endTimer);
//          }else if(key==BACKSPACE){
//             String time = nf(endTimer/1000, 10);
//             time = time.substring(0,max(0,time.length()-1));
//             endTimer = parseInt(time);
//             endTimer=endTimer*1000;
//          }
//        }
//        //we want to change the ending timer
//        
//        
//      }else if(key==ENTER || key==RETURN){
//        attemptTimer = new Timer(attTimer);
//        endingTimer = new Timer(endTimer);
//        calibrationTimer = new Timer(calTimer);
//        currentState = "CALIBRATION";
//      }
//    }else{
//      if(key=='s' || key=='S'){
//         currentState = "SETTINGS";
//         attemptTimer = new Timer(attTimer);
//         endingTimer = new Timer(endTimer);
//         calibrationTimer = new Timer(calTimer);
//       }
//    }     
//  }
}

void showScore(){
    //TODO must be improved
    if(bestScore>0){
      pushMatrix();
      fill(fontColor);
      textAlign(LEFT);
      translate(20,60);
      text(String.format("Il miglior punteggio é: %.2f / %d",bestScore, numberOfCircles),0,0);
      popMatrix();
    }
    if(lastScore>0){
      pushMatrix();
      fill(fontColor);
      textAlign(LEFT);
      translate(20,90);
      text(String.format("Hai ottenuto un punteggio di: %.2f / %d",lastScore, numberOfCircles),0,0);
      popMatrix();
    }
   
}  


void serialEvent(Serial port) {
//    interval = millis();
    while (port.available() > 0) {
        String inBuffer = port.readString();
        if (inBuffer != null) {
          if(inBuffer.equals("\n")){
            if(first==true){
              YPRdata="";
              first = false;
              return;
            }
            println(YPRdata);
            String [] YPRvalues = split(YPRdata,',');
            Float [] YPR = {0.0,0.0};
            if(YPRvalues.length >=3){
              for(int i =0; i<2;i++){
                YPR[i] = Float.parseFloat(YPRvalues[i]);
              }
              
              roll = YPR[0];
              pinch = YPR[1];
              
              char ch = YPRvalues[2].charAt(0);
              switch( ch ){
                case 'c':
                case 'C':
                  currentState = "CALIBRATION";
                  println("CALIBRATION");
                  secondsToGo = floor((float) roll);
                  println(String.format("%d",secondsToGo));
                  break;
                case 'e':
                case 'E':
                  currentState = "ENDING";
                  lastScore=(float) pinch;
                  bestScore=(float) roll;
                  
                  break;
                case 'w':
                case 'W':
                  currentState = "WAITING";
                  break;
                case 's':
                case 'S':
                  currentState = "SENDDATA";
                  break;
                case 'd':
                case 'D':
                  currentState = "COUNTDOWN";
                  secondsToGo =floor((float) roll);
                  break;
                case 'a':
                case 'A':
                  currentState = "ATTEMPT";
                  mx = (float) roll;
                  my = (float) pinch;
                  secondsToGo = floor((float) Double.parseDouble(YPRvalues[3]));
                  score = (float) Double.parseDouble(YPRvalues[4]);
                  break;
              }
            }else{
              YPRdata="";
              return;
            }                 
            YPRdata="";
          }else{
            YPRdata+=inBuffer;
          }
          
        }
    }
}
