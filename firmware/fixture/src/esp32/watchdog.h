#pragma once

#define RES_WDT_S 8

void watchdogInit(); // 8 s task watchdog, panic+reset, loop task subscribed
bool watchdogArmed();
void watchdogService(); // no-op until armed; safe inside long driver transfers
