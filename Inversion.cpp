#include "Inversion.h"
#include "Portal.h"
#include <EEPROM.h>

// Tables d'inversion pour joystick filaire et manette
bool invertJoy[AX_COUNT] = {false,false,true,false,false,false,false,false};
bool invertPad[AX_COUNT] = {false,false,false,false,false,false,false,false};

#define INV_EE_MAGIC      0x1A55
#define INV_EE_VER        0x0001
#define INV_EE_MAGIC_ADDR 292
#define INV_EE_VER_ADDR   294
#define INV_EE_DATA_ADDR  296

static const char* AX_NAMES[AX_COUNT] = {"X","Y","Z","LX","LY","LZ","R1","R2"};

void inversionSaveToEEPROM(){
#if defined(ARDUINO_ARCH_ESP32)
  EEPROM.begin(512);
#endif
  EEPROM.put(INV_EE_MAGIC_ADDR, (uint16_t)INV_EE_MAGIC);
  EEPROM.put(INV_EE_VER_ADDR, (uint16_t)INV_EE_VER);
  int addr = INV_EE_DATA_ADDR;
  for(int i=0;i<AX_COUNT;i++){ uint8_t v = invertJoy[i]?1:0; EEPROM.put(addr++, v); }
  for(int i=0;i<AX_COUNT;i++){ uint8_t v = invertPad[i]?1:0; EEPROM.put(addr++, v); }
#if defined(ARDUINO_ARCH_ESP32)
  EEPROM.commit();
#endif
}

static bool inversionLoadFromEEPROM(){
#if defined(ARDUINO_ARCH_ESP32)
  EEPROM.begin(512);
#endif
  uint16_t magic=0,ver=0;
  EEPROM.get(INV_EE_MAGIC_ADDR, magic);
  EEPROM.get(INV_EE_VER_ADDR, ver);
  if(magic!=INV_EE_MAGIC) return false;
  int addr = INV_EE_DATA_ADDR;
  for(int i=0;i<AX_COUNT;i++){ uint8_t b=0; EEPROM.get(addr++, b); invertJoy[i]=(b!=0); }
  for(int i=0;i<AX_COUNT;i++){ uint8_t b=0; EEPROM.get(addr++, b); invertPad[i]=(b!=0); }
  return true;
}

void inversionLoadOrDefault(){
  if(!inversionLoadFromEEPROM()){
    for(int i=0;i<AX_COUNT;i++){ invertJoy[i]=false; invertPad[i]=false; }
    invertJoy[AX_Z]=true; // par défaut, Z inversé pour le joystick
  }
}

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

static String pageHtml(bool joy){
  String html;
  html.reserve(4000);
  html += "<!DOCTYPE html><html><head><meta charset=\"utf-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
  html += String("<title>Inversion sens ") + (joy?"Joystick":"Manette") + "</title>";
  html += "<style>";
  html += ":root{--bg:#0f172a;--fg:#e5e7eb;--line:#1f2937;--btn:#334155;--btnh:#475569;}";
  html += "body{font-family:system-ui,Arial;margin:0;background:var(--bg);color:var(--fg);}";
  html += ".wrap{padding:16px;max-width:600px;margin:0 auto;}";
  html += "table{border-collapse:collapse;width:100%;}";
  html += "th,td{border-bottom:1px solid var(--line);padding:8px;text-align:left;}";
  html += "select{background:#111827;color:#e5e7eb;border:1px solid var(--line);border-radius:6px;padding:4px 6px;}";
  html += "button{margin-top:12px;background:var(--btn);color:var(--fg);border:1px solid var(--line);padding:8px 12px;border-radius:8px;cursor:pointer;}";
  html += "button:hover{background:var(--btnh)}";
  html += "#msg{margin-left:10px;color:#9ca3af;}";
  html += "</style></head><body><div class='wrap'>";
  html += String("<h2>Inversion sens ") + (joy?"Joystick":"Manette") + "</h2>";
  html += navBar();
  html += "<table><tr><th>Axe</th><th>Inversion</th></tr>";
  for(int i=0;i<AX_COUNT;i++){
    html += "<tr><td>"; html += AX_NAMES[i]; html += "</td><td><select id='ax"+String(i)+"'>";
    html += "<option value='0'>Non invers\u00E9</option><option value='1'>Invers\u00E9</option></select></td></tr>";
  }
  html += "</table><div class='bar'><button id='send'>Envoyer</button><button id='save'>Sauvegarde EEPROM</button><span id='msg'></span></div>";
  html += "<script>var CUR=[";
  bool *arr = joy?invertJoy:invertPad;
  for(int i=0;i<AX_COUNT;i++){ if(i) html+=","; html += (arr[i]?"1":"0"); }
  html += "];";
  html += "function init(){for(var i=0;i<8;i++){document.getElementById('ax'+i).value=CUR[i];}document.getElementById('send').onclick=function(){apply(false);};document.getElementById('save').onclick=function(){apply(true);};}";
  html += "function apply(s){var v=[];for(var i=0;i<8;i++){v.push(document.getElementById('ax'+i).value);}var msg=document.getElementById('msg');msg.textContent='Envoi...';fetch((s?'/invsave':'/invapply')+'?mode=";
  html += joy?"joy":"pad";
  html += "&v='+v.join(','),{cache:'no-store'}).then(function(r){if(!r.ok)throw new Error();return r.text();}).then(function(){msg.textContent=s?'Sauvegard\u00E9':'Appliqu\u00E9';setTimeout(function(){msg.textContent='';},1500);}).catch(function(){msg.textContent='Erreur';});}";
  html += "init();</script></div></body></html>";
  return html;
}

static void parseCSV8bool(const String& s,bool out[AX_COUNT]){
  int idx=0; String tok="";
  for(uint16_t i=0;i<=s.length();++i){
    if(i==s.length() || s[i]==','){ if(idx<AX_COUNT){ out[idx++]=(tok.toInt()!=0); } tok=""; }
    else tok+=s[i];
  }
  while(idx<AX_COUNT) out[idx++]=false;
}

void inversionMountRoutes(){
  auto& server = portalServer();
  server.on("/invjoy", HTTP_GET, [&](){ server.send(200,"text/html",pageHtml(true)); });
  server.on("/invpad", HTTP_GET, [&](){ server.send(200,"text/html",pageHtml(false)); });
  server.on("/invapply", HTTP_GET, [&](){
    if(!server.hasArg("mode") || !server.hasArg("v")){ server.send(400,"text/plain","missing"); return; }
    bool joy = (server.arg("mode") == "joy");
    bool tmp[AX_COUNT];
    parseCSV8bool(server.arg("v"), tmp);
    if(joy){ for(int i=0;i<AX_COUNT;i++) invertJoy[i]=tmp[i]; }
    else { for(int i=0;i<AX_COUNT;i++) invertPad[i]=tmp[i]; }
    server.send(200,"text/plain","OK");
  });
  server.on("/invsave", HTTP_GET, [&](){
    if(!server.hasArg("mode") || !server.hasArg("v")){ server.send(400,"text/plain","missing"); return; }
    bool joy = (server.arg("mode") == "joy");
    bool tmp[AX_COUNT];
    parseCSV8bool(server.arg("v"), tmp);
    if(joy){ for(int i=0;i<AX_COUNT;i++) invertJoy[i]=tmp[i]; }
    else { for(int i=0;i<AX_COUNT;i++) invertPad[i]=tmp[i]; }
    inversionSaveToEEPROM();
    server.send(200,"text/plain","OK");
  });
}
