// ================================================
//  Fixed Ultrasonic Radar Visualization (Processing)
//  Works with the improved Arduino code
// ================================================

import processing.serial.*;

Serial myPort;

int iAngle = 0;
int iDistance = 0;

void setup() {
  size(1200, 700);           // Change this if your screen is smaller
  smooth();
  
  // === CHANGE THIS TO YOUR ACTUAL ARDUINO PORT ===
  myPort = new Serial(this, "COM6", 9600);   // Windows example: "COM4", "COM5"...
  // Mac/Linux example: "/dev/cu.usbmodemXXXX" or "/dev/ttyUSB0"
  
  myPort.clear();   // Clear any garbage in buffer
}

void draw() {
  // Simulate motion blur / fade effect
  fill(0, 4);
  noStroke();
  rect(0, 0, width, height - height * 0.065);

  fill(98, 245, 31);   // Green color
  
  drawRadar();
  drawLine();
  drawObject();
  drawText();
}

// New, robust serial reading
void serialEvent(Serial myPort) {
  String inString = myPort.readStringUntil('\n');   // Read full line
  
  if (inString != null) {
    inString = trim(inString);                     // Remove \n and spaces
    
    String[] data = split(inString, ',');          // Split "angle,distance"
    
    if (data.length == 2) {
      int newAngle = int(data[0]);
      int newDistance = int(data[1]);
      
      // Basic validation
      if (newAngle >= 0 && newAngle <= 200) {
        iAngle = newAngle;
      }
      if (newDistance >= 0) {
        iDistance = newDistance;
      }
    }
  }
}

void drawRadar() {
  pushMatrix();
  translate(width/2, height - height*0.074);
  noFill();
  strokeWeight(2);
  stroke(98, 245, 31);
  
  // Distance arcs
  arc(0, 0, width*0.9375, width*0.9375, PI, TWO_PI);
  arc(0, 0, width*0.73,   width*0.73,   PI, TWO_PI);
  arc(0, 0, width*0.521,  width*0.521,  PI, TWO_PI);
  arc(0, 0, width*0.313,  width*0.313,  PI, TWO_PI);
  
  // Angle lines
  line(-width/2, 0, width/2, 0);
  for (int a = 30; a <= 150; a += 30) {
    line(0, 0, (-width/2)*cos(radians(a)), (-width/2)*sin(radians(a)));
  }
  popMatrix();
}

void drawObject() {
  pushMatrix();
  translate(width/2, height - height*0.074);
  strokeWeight(9);
  stroke(255, 10, 10);   // Red
  
  float pixsDistance = iDistance * ((height - height*0.1666) * 0.025);
  
  if (iDistance < 40 && iDistance > 0) {
    line(pixsDistance * cos(radians(iAngle)), 
         -pixsDistance * sin(radians(iAngle)), 
         (width - width*0.505) * cos(radians(iAngle)), 
         -(width - width*0.505) * sin(radians(iAngle)));
  }
  popMatrix();
}

void drawLine() {
  pushMatrix();
  translate(width/2, height - height*0.074);
  strokeWeight(9);
  stroke(30, 250, 60);   // Bright green sweep
  
  line(0, 0, 
       (height - height*0.12) * cos(radians(iAngle)), 
       -(height - height*0.12) * sin(radians(iAngle)));
  popMatrix();
}

void drawText() {
  pushMatrix();
  
  // Bottom black bar
  fill(0);
  noStroke();
  rect(0, height - height*0.0648, width, height);
  
  fill(98, 245, 31);
  textSize(25);
  
  // Distance markers
  text("10cm", width*0.6146, height - height*0.0833);
  text("20cm", width*0.719,  height - height*0.0833);
  text("30cm", width*0.823,  height - height*0.0833);
  text("40cm", width*0.9271, height - height*0.0833);
  
  textSize(40);
  text("Basilus's Radar", width*0.125, height - height*0.0277);
  text("Angle: " + iAngle + " °", width*0.52, height - height*0.0277);
  
  text("Distance: ", width*0.70, height - height*0.0277);
  if (iDistance < 40 && iDistance > 0) {
    text(iDistance + " cm", width*0.775, height - height*0.0277);
  } else {
    text("Out of Range", width*0.775, height - height*0.0277);
  }
  
  // Angle labels on radar
  textSize(25);
  fill(98, 245, 60);
  
  // 30°
  pushMatrix();
  translate(width/2 + (width/2)*cos(radians(30)), height*0.91 - (width/2)*sin(radians(30)));
  rotate(radians(-60));
  text("30°", 0, 0);
  popMatrix();
  
  // 60°
  pushMatrix();
  translate(width/2 + (width/2)*cos(radians(60)), height*0.91 - (width/2)*sin(radians(60)));
  rotate(radians(-30));
  text("60°", 0, 0);
  popMatrix();
  
  // 90°
  text("90°", width/2 - 15, height*0.83);
  
  // 120°
  pushMatrix();
  translate(width/2 + (width/2)*cos(radians(120)), height*0.91 - (width/2)*sin(radians(120)));
  rotate(radians(30));
  text("120°", 0, 0);
  popMatrix();
  
  // 150°
  pushMatrix();
  translate(width/2 + (width/2)*cos(radians(150)), height*0.91 - (width/2)*sin(radians(150)));
  rotate(radians(60));
  text("150°", 0, 0);
  popMatrix();
  
  popMatrix();
}
