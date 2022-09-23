//
// Created by Lukas on 22/07/2022.
//

#include <unistd.h>
#include <pthread.h>
#include "main.h"
#include "includes/cipher/Cipher.h"
#include "includes/imgui/imgui.h"
#include "includes/misc/Logger.h"


float* framerate = NULL;
bool NoInit = true;
_Atomic bool hasInitFinished = false;



void* find(char* start, size_t size, char* what, size_t howMuch) {
    if(size < howMuch) return NULL;
    for(size_t i = 0; i < size; i++) {
        for(size_t j = 0; j < howMuch; j++) {
            if(start[i+j] != what[j]) break;
            if(i+j >= size) break;
            if(j == howMuch - 1) return &start[i];
        }
    }
    return NULL;
}
void* find_framerate(void* args) {
    ImVec2 vec = *((ImVec2*)args);
    __android_log_print(ANDROID_LOG_INFO,"FramerateChanger","display resolution: %.3f %.3f", vec.x, vec.y);
    float what[2];
    what[0] = vec.x;
    what[1] = vec.y;
    __android_log_print(ANDROID_LOG_INFO,"FramerateChanger","FramerateSearch: %p", *((long*)what));
    FILE* file;
    char* line = NULL;
    size_t len = 0;
    const char* mapping_name_prefix = android_get_device_api_level() < 30 ? "[anon:libc_malloc" : "[anon:scudo:";
    const int mapping_name_len = strlen(mapping_name_prefix);
    file = fopen("/proc/self/maps", "r");
    if(file == NULL) {
        hasInitFinished = true;
        pthread_exit(NULL);
    }
    bool NotFound = true;
    while (NotFound && getline(&line, &len, file) != -1) {
        char* searchstart = strchr(line, '[');
        if(searchstart == NULL) continue;
        if(!strncmp(searchstart, mapping_name_prefix, mapping_name_len)) {
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
        }
    }
    free(line);
    fclose(file);
    hasInitFinished = true;
    pthread_exit(NULL);
}

void Menu() {
    if(NoInit) {
        pthread_t p;
        pthread_create(&p, NULL, &find_framerate, &ImGui::GetIO().DisplaySize);
        NoInit = false;
    }
    if(hasInitFinished) {
        if(framerate != NULL) {
            ImGui::Text("%s", "Maximum framerate");
            ImGui::SliderFloat("", framerate, 0.0f, 120.0f);
        }else {
            ImGui::Text("%s", "Failed to initialize");
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