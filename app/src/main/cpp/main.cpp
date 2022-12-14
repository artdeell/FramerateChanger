//
// Created by Lukas on 22/07/2022.
//

#include <unistd.h>
#include <pthread.h>
#include "main.h"
#include "includes/cipher/Cipher.h"
#include "includes/imgui/imgui.h"
#include "includes/misc/Logger.h"


float* framerate = nullptr;
bool NoInit = true;
_Atomic bool hasInitFinished = false;
char initFailReason[1024] = {0};
const char* scudo_search_p = "[anon:scudo:";
const size_t scudo_search_s = strlen(scudo_search_p);
const char* libc_search_p = "[anon:libc_malloc";
const size_t libc_search_s = strlen(libc_search_p);



void* find(char* start, size_t size, const char* what, size_t howMuch) {
    if(size < howMuch) return nullptr;
    for(size_t i = 0; i < size; i++) {
        for(size_t j = 0; j < howMuch; j++) {
            if(i+j >= size) return nullptr;
            if(start[i+j] != what[j]) break;
            if(j == howMuch - 1) return &start[i];
        }
    }
    return nullptr;
}
void* find_framerate(void* args) {
    ImVec2 vec = *((ImVec2*)args);
    __android_log_print(ANDROID_LOG_INFO,"FramerateChanger","display resolution: %.3f %.3f", vec.x, vec.y);
    float what[2];
    what[0] = vec.x;
    what[1] = vec.y;
    FILE* file;
    char* line = nullptr;
    size_t len = 0;
    file = fopen("/proc/self/maps", "r");
    if(file == nullptr) {
        strcpy(initFailReason,  "Failed to read procmaps");
        hasInitFinished = true;
        pthread_exit(nullptr);
    }
    bool NotFound = true;
    bool scudoPageFound = false;
    while (NotFound && getline(&line, &len, file) != -1) {
        char* searchstart = strchr(line, '[');
        if(searchstart == nullptr) continue;
        if(!strncmp(searchstart,scudo_search_p, scudo_search_s) || !strncmp(searchstart,libc_search_p, libc_search_s)) {
            char*end0 = nullptr;
            unsigned long addr0 = strtoul(line, &end0, 0x10);
            char*end1 = nullptr;
            unsigned long addr1 = strtoul(end0+1, &end1, 0x10);
            void* found = find((char*)addr0, addr1-addr0-1, (char *) what, sizeof what);
            while(found) {
                float *found_framerate = &((float*)found)[2];
                if(*found_framerate == 20 || *found_framerate == 30 || *found_framerate == 60) {
                    framerate = found_framerate;
                    NotFound = false;
                    break;
                }else{
                    addr0 = ((unsigned  long)found)+1;
                }
                found = find((char*)addr0, addr1-addr0-1, (char *) what, sizeof what);
            }
            scudoPageFound = true;
        }
    }
    scudoPageFound ? strcpy(initFailReason, "Address not found") : strcpy(initFailReason, "No scudo pages");
    free(line);
    fclose(file);
    hasInitFinished = true;
    pthread_exit(nullptr);
}

void Menu() {
    if(NoInit) {
        pthread_t p;
        pthread_create(&p, nullptr, &find_framerate, &ImGui::GetIO().DisplaySize);
        NoInit = false;
    }
    if(hasInitFinished) {
        if(framerate != nullptr) {
            ImGui::Text("%s", "Maximum framerate");
            ImGui::SliderFloat("", framerate, 0.0f, 120.0f);
        }else {
            ImGui::Text("Failed to initialize (%s)", initFailReason);
            if(ImGui::Button("Retry")) {
                hasInitFinished = false;
                NoInit = true;
            }
        }
    }else{
        ImGui::Text("%s", "Initializing...");
    }
}

void Init(){

};
