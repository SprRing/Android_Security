#include <stdlib.h>

#include "nodeobject.h"
#include<curl/curl.h>
#include<string.h>

char global[7000000];
char *array[20000];
int len;

size_t static curl_write(void *buffer, size_t size, size_t nmemb, void *userp)
{
     userp += strlen(userp);  // Skipping to first unpopulated char
     memcpy(userp, buffer, nmemb);  // Populating it.
     return nmemb;
}

size_t static write_callback_func(void *buffer,
                        size_t size,
                        size_t nmemb,
                        void *userp)
{
    char **response_ptr =  (char**)userp;

    /* assuming the response is a string */
    *response_ptr = strndup(buffer, (size_t)(size *nmemb));
    //printf("well well = %s\n",buffer);
    strcat(global,buffer);
    return ((size_t)(size *nmemb));
    
//if you not send return value of size it will show you ERROR CODE 23return  curl_easy_perform();

}

static  O1 * callAPI (String* p) {
    CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  char concater[7000000];
  char *response= (char *) malloc(50000);
  strcpy(concater, "http://18.191.126.212:8888/fetchMethods2/?perm=");
  while(*p)
  {
    strcat(concater,*p);
    strcat(concater,";");
    *p++;
  }



  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL,concater);
    /* example.com is redirected, so we tell libcurl to follow redirection */
    
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_func);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    //curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_func);
    //curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK)
    {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
              curl_easy_cleanup(curl);
              return("Fail");
    }

    else
    {
    /* always cleanup */
    curl_easy_cleanup(curl);
    //return(res);
    //printf("curl response check:\n");
      //printf("%s\n",response);
      
        int i =0;  
        char * token = strtok(global, ";");
            while( token != NULL ) 
            {
            token = strtok(NULL, ";");   
            if(token!=NULL)
            array[i] = token;
            //printf( " %s\n", array[i]); 
            i++;
            len++;
            }
    return(array);
    }
  }

  //return ;
}

O1 * newO1(String id) {
    O1 * ret = calloc(1, sizeof(O1));
    ret->callAPI = callAPI;
    return ret;
}

void testO1() {
    char *ans=(char *) malloc(50000);
    O1 * o = newO1("callerObj");
    char * str[5];
    str[0] = "cache";
    str[1] = "bluetooth";
    str[2] = "camera";
    ans = o->callAPI(str);

    //length of the array
    printf("Length of array");
    printf("%d\n items",len);
    
    //sample array data 
    for(int i=0;i<10;i++)
    {
      printf("%s\n",array[i]);
    }
    

}

void main()
{
    testO1();
}