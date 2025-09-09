#include "ValveCalib.h"
#include "IOMap.h"
#include "Portal.h"
#include <EEPROM.h>

static uint16_t evNeutral = 512;
static uint16_t evMin = 255;
static uint16_t evMax = 768;

#define EV_EE_MAGIC      0xE1A1
#define EV_EE_VER        0x0001
#define EV_EE_MAGIC_ADDR 312
#define EV_EE_VER_ADDR   314
#define EV_EE_NEU_ADDR   316
#define EV_EE_MIN_ADDR   318
#define EV_EE_MAX_ADDR   320

void valveCalibLoad(){
  loadNeutralOffset();
#if defined(ARDUINO_ARCH_ESP32)
  EEPROM.begin(512);
#endif
  uint16_t magic=0, ver=0;
  EEPROM.get(EV_EE_MAGIC_ADDR, magic);
  EEPROM.get(EV_EE_VER_ADDR, ver);
  if(magic==EV_EE_MAGIC){
    EEPROM.get(EV_EE_NEU_ADDR, evNeutral);
    EEPROM.get(EV_EE_MIN_ADDR, evMin);
    EEPROM.get(EV_EE_MAX_ADDR, evMax);
    neutralOffset = evNeutral;
    valveMin = evMin;
    valveMax = evMax;
  } else {
    evNeutral = neutralOffset;
    evMin = valveMin;
    evMax = valveMax;
  }
  updateNeutralWindow();
}

static void valveCalibSaveToEEPROM(){
#if defined(ARDUINO_ARCH_ESP32)
  EEPROM.begin(512);
#endif
  EEPROM.put(EV_EE_MAGIC_ADDR, (uint16_t)EV_EE_MAGIC);
  EEPROM.put(EV_EE_VER_ADDR,   (uint16_t)EV_EE_VER);
  EEPROM.put(EV_EE_NEU_ADDR,   evNeutral);
  EEPROM.put(EV_EE_MIN_ADDR,   evMin);
  EEPROM.put(EV_EE_MAX_ADDR,   evMax);
#if defined(ARDUINO_ARCH_ESP32)
  EEPROM.commit();
#endif
  saveNeutralOffset();
}

static inline int clampInt(int v,int lo,int hi){ if(v<lo) return lo; if(v>hi) return hi; return v; }

static String navBar(){
  return String(
    "<div class='bar'>"
      "<a class='btn' href='/defaut'>D\u00E9faut ESP32</a>"
      "<a class='btn' href='/calib'>Calibration Joystick</a>"
      "<a class='btn' href='/calev'>Calibration \u00E9lectrovannes</a>"
      "<a class='btn' href='/bridage'>Bridage axes</a>"
      "<a class='btn' href='/invjoy'>Inversion Joystick</a>"
      "<a class='btn' href='/invpad'>Inversion Manette</a>"
    "</div>"
    "<style>.bar{display:flex;flex-wrap:wrap;gap:8px;margin:10px 0 16px}"
    ".btn{background:#334155;color:#e5e7eb;border:1px solid #1f2937;padding:8px 12px;border-radius:8px;text-decoration:none}"
    ".btn:hover{background:#475569}</style>"
  );
}

static String htmlPage(){
  String html;
  html.reserve(6000);
  html += "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>Calibration \u00E9lectrovannes</title><style>";
  html += "body{font-family:system-ui,Arial;margin:16px;background:#0f172a;color:#e5e7eb}";
  html += "button{background:#334155;color:#e5e7eb;border:1px solid #1f2937;padding:8px 12px;border-radius:8px;cursor:pointer;margin-right:8px;}";
  html += "button:hover{background:#475569}";
  html += "input[type=range]{width:100%;margin:12px 0;}";
  html += "#msg,#msg2{color:#9ca3af;margin-left:10px}";
  html += "</style></head><body><h2>Calibration \u00E9lectrovannes</h2>";
  html += navBar();
  html += "<div id='step0'><button id='start'>D\u00E9marrer la calibration</button></div>";
  html += "<div id='count' style='display:none'>D\u00E9marrage dans <span id='cd'>5</span>s <button id='cancel'>Annuler</button></div>";
  html += "<div id='cal' style='display:none'>";
  html += "<p id='phase'></p><input type='range' id='sl' min='0' max='1023' value='512'>";
  html += "<div><button id='minus'>-</button><button id='plus'>+</button><button id='send'>Envoyer</button><button id='next'>Enregistrer</button><span id='msg'></span></div>";
  html += "</div><div id='save' style='display:none'><button id='saveBtn'>Sauvegarde</button><span id='msg2'></span></div>";
  html += "<script>var ph=0;var labels=['Neutre','Mini','Maxi'];function upd(){document.getElementById('phase').textContent='Phase '+(ph+1)+' - '+labels[ph];}function start(){document.getElementById('step0').style.display='none';document.getElementById('count').style.display='block';var c=5;document.getElementById('cd').textContent=c;var t=setInterval(function(){c--;document.getElementById('cd').textContent=c;if(c<=0){clearInterval(t);document.getElementById('count').style.display='none';document.getElementById('cal').style.display='block';fetch('/calev/start');upd();}},1000);document.getElementById('cancel').onclick=function(){clearInterval(t);document.getElementById('count').style.display='none';document.getElementById('step0').style.display='block';};}document.getElementById('start').onclick=start;document.getElementById('plus').onclick=function(){var s=document.getElementById('sl');s.value=Math.min(1023,parseInt(s.value)+1);};document.getElementById('minus').onclick=function(){var s=document.getElementById('sl');s.value=Math.max(0,parseInt(s.value)-1);};document.getElementById('send').onclick=function(){var v=document.getElementById('sl').value;var m=document.getElementById('msg');m.textContent='Envoi...';fetch('/calev/set?v='+v).then(function(){m.textContent='Reçu';setTimeout(function(){m.textContent='';},1500);});};document.getElementById('next').onclick=function(){var v=document.getElementById('sl').value;fetch('/calev/record?phase='+ph+'&val='+v).then(function(){ph++;if(ph<3){upd();}else{document.getElementById('cal').style.display='none';document.getElementById('save').style.display='block';}});};document.getElementById('saveBtn').onclick=function(){var m=document.getElementById('msg2');m.textContent='Envoi...';fetch('/calev/save').then(function(){m.textContent='Sauvegard\u00E9';setTimeout(function(){m.textContent='';},1500);});};</script>";
  html += "</body></html>";
  return html;
}

void valveCalibMountRoutes(){
  auto& server = portalServer();
  server.on("/calev", HTTP_GET, [](){ server.send(200,"text/html",htmlPage()); });
  server.on("/calev/start", HTTP_GET, [](){ neutralizeAllOutputs(); server.send(200,"text/plain","OK"); });
  server.on("/calev/set", HTTP_GET, [](){
    if(!server.hasArg("v")){ server.send(400,"text/plain","missing"); return; }
    int v = clampInt(server.arg("v").toInt(),0,1023);
    applyAxisToPair(0,v);
    server.send(200,"text/plain","OK");
  });
  server.on("/calev/record", HTTP_GET, [](){
    if(!server.hasArg("phase") || !server.hasArg("val")){ server.send(400,"text/plain","missing"); return; }
    int ph = server.arg("phase").toInt();
    int v = clampInt(server.arg("val").toInt(),0,1023);
    if(ph==0) evNeutral=v; else if(ph==1) evMin=v; else if(ph==2) evMax=v;
    server.send(200,"text/plain","OK");
  });
  server.on("/calev/save", HTTP_GET, [](){
    neutralOffset = evNeutral;
    valveMin = evMin;
    valveMax = evMax;
    updateNeutralWindow();
    valveCalibSaveToEEPROM();
    server.send(200,"text/plain","OK");
  });
}

