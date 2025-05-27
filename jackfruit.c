#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "cJSON.h"

typedef struct {
    char *memory;
    size_t size;
} MemoryStruct;

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    MemoryStruct *mem = (MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
        fprintf(stderr, "Not enough memory (realloc returned NULL)\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

void print_instructions_step_by_step(const char *instructions) {
    char *instr_copy = strdup(instructions);
    if (!instr_copy) {
        printf("Memory allocation failed.\n");
        return;
    }

    printf("\nInstructions (press Enter for next step):\n");

    char *step = strtok(instr_copy, ".");
    int step_num = 1;
    while (step != NULL) {
        while (*step == ' ' || *step == '\t') step++;
        if (*step) {
            printf("Step %d: %s.\n", step_num, step);
            step_num++;
            printf("Press Enter to continue...");
            getchar(); 
        }
        step = strtok(NULL, ".");
    }
    free(instr_copy);
    printf("Enjoy your food!\n");
}

int main(void) {
    CURL *curl;
    CURLcode res;
    MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    char keyword[100];
    printf("Enter a food keyword: ");
    fgets(keyword, sizeof(keyword), stdin);
    size_t len = strlen(keyword);
    if (len > 0 && keyword[len - 1] == '\n') {
        keyword[len - 1] = '\0';
    }

    char *encoded_keyword = NULL;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        encoded_keyword = curl_easy_escape(curl, keyword, 0);

        char url[512];
        snprintf(url, sizeof(url),
            "https://api.api-ninjas.com/v1/recipe?query=%s&limit=1", encoded_keyword);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "X-Api-Key: YOUR_api_KEY");
        

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "my-recipe-app/1.0");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            cJSON *json = cJSON_Parse(chunk.memory);
            if (!json) {
                fprintf(stderr, "JSON parse error: %s\n", cJSON_GetErrorPtr());
            } else if (cJSON_IsArray(json)) {
                cJSON *item = cJSON_GetArrayItem(json, 0);
                if (item) {
                    cJSON *title = cJSON_GetObjectItemCaseSensitive(item, "title");
                    cJSON *ingredients = cJSON_GetObjectItemCaseSensitive(item, "ingredients");
                    cJSON *instructions = cJSON_GetObjectItemCaseSensitive(item, "instructions");

                    if (cJSON_IsString(title) && cJSON_IsString(ingredients) && cJSON_IsString(instructions)) {
                        printf("\nRecipe: %s\n", title->valuestring);
                        printf("Ingredients: %s\n", ingredients->valuestring);
                        print_instructions_step_by_step(instructions->valuestring);
                    } else {
                        printf("Recipe details missing in the response.\n");
                    }
                } else {
                    printf("No recipe found for '%s'.\n", keyword);
                }
                cJSON_Delete(json);
            } else {
                printf("Unexpected JSON format.\n");
            }
        }

        curl_free(encoded_keyword);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    } else {
        fprintf(stderr, "Failed to initialize curl.\n");
    }

    free(chunk.memory);
    curl_global_cleanup();
    return 0;
}


