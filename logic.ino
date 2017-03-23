float tmpYaw;

void driveToWallState() {
  setLeftMotorSpeed(L_FWD_MAX);
  setRightMotorSpeed(R_FWD_MAX);
  tmpYaw = yaw;

  if (pitch > 60.0f) {
    GLOBAL_STATE = ST_UP_WALL;
  }
}

void upWallState() {
  setLeftMotorSpeed(L_FWD_MAX);
  setRightMotorSpeed(R_FWD_MAX);

  if (pitch < 30.0f) {
    GLOBAL_STATE = ST_TOP_WALL;
  }
}

void topWallState() {
  setLeftMotorSpeed(L_FWD_SLOW);
  setRightMotorSpeed(R_FWD_SLOW);

  if (pitch < -45.0f) {
    GLOBAL_STATE = ST_DOWN_WALL_1;
    STATE_START_TIME = millis();
  }
}

void downWall1State() {
  setLeftMotorSpeed(L_FWD_SLOW);
  setRightMotorSpeed(R_FWD_SLOW);

  if (millis() - STATE_START_TIME > 2000) {
    GLOBAL_STATE = ST_DOWN_WALL_2;
  }
}

void downWall2State() {
  setLeftMotorSpeed(L_FWD_MAX);
  setRightMotorSpeed(R_FWD_MAX);

  if (pitch > -10.0f) {
    GLOBAL_STATE = ST_PRE_POLE_DETECTION_1;
    STATE_START_TIME = millis();
  }
}

void prePoleDetection1() {
  setLeftMotorSpeed(L_FWD_MAX);
  setRightMotorSpeed(R_FWD_MAX);

  if (millis() - STATE_START_TIME > 300) {
    GLOBAL_STATE = ST_PRE_POLE_DETECTION_2;
  }
}

void prePoleDetection2() {
  if (fabs(tmpYaw - yaw) < 2.0f) {
    STATE_START_TIME = millis();
    GLOBAL_STATE = ST_PRE_POLE_DETECTION_3;
    setLeftMotorSpeed(92);
    setRightMotorSpeed(92);
    return;
  }

  if (yaw < tmpYaw) {
      setLeftMotorSpeed(L_REV_25);
      setRightMotorSpeed(R_FWD_SLOW);
  } else if (yaw > tmpYaw) {
      setLeftMotorSpeed(L_FWD_SLOW);
      setRightMotorSpeed(R_REV_25);
  }
}

void prePoleDetection3() {
  setLeftMotorSpeed(92);
  setRightMotorSpeed(92);

  if (millis() - STATE_START_TIME > 200) {
    GLOBAL_STATE = ST_PRE_POLE_DETECTION_4;
  }
}

void prePoleDetection4() {
  setLeftMotorSpeed(L_FWD_50);
  setRightMotorSpeed(R_FWD_50);

  if ((getCurrentLeftMotorSpeed() == L_FWD_50) and (getCurrentRightMotorSpeed() == R_FWD_50)) {
    GLOBAL_STATE = ST_POLE_DETECT;
  }
}

void poleDection(){
  setLeftMotorSpeed(L_FWD_50);
  setRightMotorSpeed(R_FWD_50);
  getUSDis();
  printAllUSDiff();
  printAllUSValues();

  int leftDiff = leftDis - leftDis_old;
  int rightDiff = rightDis - rightDis_old;

  if(rightDiff > -1*(CM_WALL_TO_RAMP+DETECT_TOLERANCE) && rightDiff < -1*(CM_WALL_TO_RAMP-DETECT_TOLERANCE)&& rightDis!= 0){
    Serial.println("ramp on left");
  }

  if(leftDiff + CM_POLE_TO_WALL < DETECT_TOLERANCE && leftDis!= 0 ){
    //track 90 - 15/2 = 82.5deg
    targetYaw = yaw + 82.5f;
    GLOBAL_STATE = ST_TURN_TOWARD_POLE;
    Serial.println("pole on left");
    Serial.println(leftDiff);
    Serial.println(-1*(CM_POLE_TO_WALL-DETECT_TOLERANCE));
  }

  if(rightDiff <= (-1*(CM_POLE_TO_WALL-DETECT_TOLERANCE)) && rightDis != 0){
    //track -(90 - 15/2) = -82.5deg
    targetYaw = yaw - 82.5f;
    GLOBAL_STATE = ST_TURN_TOWARD_POLE;
    Serial.println("pole on right");
    Serial.println(rightDiff);
    Serial.println(-1*(CM_POLE_TO_WALL-DETECT_TOLERANCE));
  }

  if(targetYaw > 180)
    targetYaw -= 360.0f;
  if(targetYaw < -180)
    targetYaw += 360.0f;
}

void turnTowardPole(){
  float diff = yawDiff();
  if(fabs(diff)< 2.0f){
    setLeftMotorSpeed(L_STOP);
    setRightMotorSpeed(R_STOP);
    GLOBAL_STATE = ST_DRIVE_TO_POLE;
  }
  else if(targetYaw > yaw){
    Serial.print("    Turning left");
    setLeftMotorSpeed(L_REV_50);
    setRightMotorSpeed(R_FWD_50);
    if(fabs(diff)<60.0f){
      setLeftMotorSpeed(L_REV_25);
      setRightMotorSpeed(R_FWD_SLOW);
    }
  }
  else if(targetYaw < yaw){
    Serial.print("    Turning Right");
    setLeftMotorSpeed(L_FWD_50);
    setRightMotorSpeed(R_REV_50);
    if(fabs(diff)<60.0f){
      setLeftMotorSpeed(L_FWD_SLOW);
      setRightMotorSpeed(R_REV_25);
    }
  }
  else{
    setLeftMotorSpeed(L_STOP);
    setRightMotorSpeed(R_STOP);
    Serial.print("YOU FKED UP");
  }
  Serial.println();
}

void driveToPole(){
  float diff = yawDiff();
  if(fabs(diff)< 5.0f){
//    setLeftMotorSpeed(L_STOP);
//    setRightMotorSpeed(R_STOP);
    setLeftMotorSpeed(L_FWD_MAX);
    setRightMotorSpeed(R_FWD_MAX);
  }
  else{
    GLOBAL_STATE = ST_TURN_TOWARD_POLE;
  }
}

float yawDiff(){
  float diff = targetYaw - yaw;
  Serial.print("targetYaw = ");
  Serial.print(targetYaw);
  Serial.print("  yaw = ");
  Serial.print(yaw);
  Serial.print("  difference in yaw = ");
  Serial.println(diff);
  return diff;
}

