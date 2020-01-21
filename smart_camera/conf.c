#include"conf.h"
#include<stdio.h>
#include<stdlib.h>
#include "tjstal.h"
#include <string.h>
#define out 
/*
 *去除字符串右端空格
 */
char *strtrimr(char *pstr)
{
    int i;
    i = strlen(pstr) - 1;
    while (isspace(pstr[i]) && (i >= 0))
        pstr[i--] = '\0';
    return pstr;
}
/*
 *去除字符串左端空格
 */
char *strtriml(char *pstr)
{
    int i = 0,j;
    j = strlen(pstr) - 1;
    while (isspace(pstr[i]) && (i <= j))
        i++;
    if (0<i)
        strcpy(pstr, &pstr[i]);
    return pstr;
}
/*
 *去除字符串两端空格
 */
char *strtrim(char *pstr)
{
    char *p;
    p = strtrimr(pstr);
    return strtriml(p);
}


/*
 *从配置文件的一行读出key或value,返回item指针
 *line--从配置文件读出的一行
 */
int  get_item_from_line(char *line, out ITEM *item)
{
    char *p = strtrim(line);
    int len = strlen(p);
    if(len <= 0){
        return 1;//空行
    }
    else if(p[0]=='#'){
        return 2;
    }else{
        char *p2 = strchr(p, '=');
        *p2++ = '\0';
        item->key = (char *)malloc(strlen(p) + 1);
        item->value = (char *)malloc(strlen(p2) + 1);
        strcpy(item->key,p);
        strcpy(item->value,p2);

        }
    return 0;//查询成功
}

int file_to_items(const char *file, out ITEM *items, out int *num)
{
    char line[MAX_CONF_ITEMS];
    FILE *fp;
    fp = fopen(file,"r");
    if(!fp)
        return 1;
    int i = 0;
    while(fgets(line, MAX_CONF_ITEMS, fp)){
            char *p = strtrim(line);
        int len = strlen(p);
        if(len <= 0){
            continue;
        }
        else if(p[0]=='#'){
            continue;
        }else{
            char *p2 = strchr(p, '=');
            /*这里认为只有key没什么意义*/
            if(!p2)
                continue;
            *p2++ = '\0';
            items[i].key = (char *)malloc(strlen(p) + 1);
            items[i].value = (char *)malloc(strlen(p2) + 1);
            strcpy(items[i].key,p);
            strcpy(items[i].value,p2);

            i++;
        }
    }
    (*num) = i;
    fclose(fp);
    return 0;
}

/*
 *读取value
 */
int read_conf_value(const char *key,out char *value,const char *file)
{
    char line[MAX_CONF_ITEMS];
    FILE *fp;
    fp = fopen(file,"r");
    if(!fp)
        return 1;//文件打开错误
    while (fgets(line, 1023, fp)){
        ITEM item;
        get_item_from_line(line,&item);
        if(!strcmp(item.key,key)){
            strcpy(value,item.value);

            fclose(fp);
            free(item.key);
            free(item.value);
            break;
        }

    }
    return 0;//成功
}

int write_conf_value(const char *key,char *value,const char *file)
{
    ITEM items[MAX_CONF_ITEMS];// 假定配置项最多有20个
    int num;//存储从文件读取的有效数目
    file_to_items(file, items, &num);

    int i=0;
    //查找要修改的项
    for(i=0;i<num;i++){
        if(!strcmp(items[i].key, key)){
            items[i].value = value;
            break;
        }
    }

    // 更新配置文件,应该有备份，下面的操作会将文件内容清除
    FILE *fp;
    fp = fopen(file, "w");
    if(!fp)
        return 1;

    i=0;
    for(i=0;i<num;i++){
        fprintf(fp,"%s=%s\n",items[i].key, items[i].value);
        //printf("%s=%s\n",items[i].key, items[i].value);
    }
    fclose(fp);
    //清除工作
/*    i=0;
    for(i=0;i<num;i++){
        free(items[i].key);
        free(items[i].value);
    }*/

    return 0;

}

int is_recording = 1;
int is_running_algorithm = 1;
#define CONFIG_FILE_PATH "/etc/camera.conf"
void config_init()
{
	char temp[256];
    int len;
	if(!read_conf_value("IS_RECORDING",temp,CONFIG_FILE_PATH))
		is_recording = strcmp(temp,"TRUE")?0:1;
	else
		printf("read config file error\n");	
	if(!read_conf_value("IS_RUNNING_ALGORITHM",temp,CONFIG_FILE_PATH))
		is_running_algorithm = strcmp(temp,"TRUE")?0:1;
	else
		printf("read config file error\n");	

    memset(temp, 0, 256);
    if(!read_conf_value("COMPANY_NAME", temp, CONFIG_FILE_PATH)){
        len = strlen(temp);
        strncpy(base_Info.company_name, temp, len);
        base_Info.company_name_length = len;
        printf("%s\r\n", temp);
    }

    memset(temp, 0, 256);
    if(!read_conf_value("PRODUCT_CODE", temp, CONFIG_FILE_PATH)){
        len = strlen(temp);
        strncpy(base_Info.product_code, temp, len);
        base_Info.product_code_length = len;
        printf("%s\r\n", temp);
    }

    memset(temp, 0, 256);
    if(!read_conf_value("HARDWARE_VERSION", temp, CONFIG_FILE_PATH)){
        len = strlen(temp);
        strncpy(base_Info.hardware_version, temp, len);
        base_Info.hardware_version_length = len;
        printf("%s\r\n", temp);
    }

    memset(temp, 0, 256);
    if(!read_conf_value("SOFTWARE_VERSION", temp, CONFIG_FILE_PATH)){
        len = strlen(temp);
        strncpy(base_Info.software_version, temp, len);
        base_Info.software_version_length = len;
        printf("%s\r\n", temp);
    }

    memset(temp, 0, 256);
    if(!read_conf_value("MACHINE_ID", temp, CONFIG_FILE_PATH)){
        len = strlen(temp);
        strncpy(base_Info.machine_id, temp, len);
        base_Info.machine_id_length = len;
        printf("%s\r\n", temp);
    }

    memset(temp, 0, 256);
    if(!read_conf_value("CUSTOMER_CODE", temp, CONFIG_FILE_PATH)){
        len = strlen(temp);
        strncpy(base_Info.customer_code, temp, len);
        base_Info.customer_code_length = len;
        printf("%s\r\n", temp);
    }
}


