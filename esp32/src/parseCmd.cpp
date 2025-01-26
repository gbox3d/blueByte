#include <Arduino.h>

#include <vector>

#include "tonkey.hpp"

#include "config.hpp"


tonkey g_MainParser;

extern Config g_config;

String parseCmd(String _strLine)
{

    JsonDocument _res_doc;

    g_MainParser.parse(_strLine);
    if (g_MainParser.getTokenCount() > 0)
    {
        String cmd = g_MainParser.getToken(0);

        if (cmd == "about")
        {
            /* code */
            _res_doc["result"] = "ok";
            _res_doc["os"] = "cronos-v1";
            _res_doc["app"] = "example-00 cli";
            _res_doc["version"] = "1.0.1";
            _res_doc["author"] = "gbox3d";
// esp8266 chip id
#ifdef ESP8266
            _res_doc["chipid"] = ESP.getChipId();
#elif ESP32
            _res_doc["chipid"] = ESP.getEfuseMac();
#endif
        }
        else if (cmd == "reboot")
        {
#ifdef ESP8266
            ESP.reset();
#elif ESP32
            ESP.restart();
#endif
        }
        else if (cmd == "config")
        {
            std::vector<String> tokens;
            for (int i = 0; i < g_MainParser.getTokenCount(); i++)
            {
                tokens.push_back(g_MainParser.getToken(i));
            }
            g_config.parseCmd(tokens, _res_doc);
        }

        else
        {
            _res_doc["result"] = "fail";
            _res_doc["ms"] = "unknown command";
        }
    }
    else
    {
        _res_doc["result"] = "fail";
        _res_doc["ms"] = "need command";
    }

    return _res_doc.as<String>();
}