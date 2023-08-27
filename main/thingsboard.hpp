#pragma once

bool isThingsBoardConnected(void);
void thingsBoardLoop(void);
void thingsBoardClientAttributesrequestSend(void);
bool getClientAttribute(unsigned long* todayTotalStudyTimePtr, unsigned long* todayTotalStudyTimeTimeStampPtr);
void thingsBoardSendTelemetry(const char *str);
void thingsBoardSendTelemetryInt(const char *key,long value);
void thingsBoardSendTelemetryBool(const char *key,bool value);
void thingsBoardSendAttributeInt(const char *key,long value);
void connectToThingsBoard(void);
void telemetry_upload__main(void);
