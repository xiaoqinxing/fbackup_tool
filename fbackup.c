#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/stat.h"

typedef unsigned char uint8_t;
typedef struct fBackupMode
{
    uint8_t source;
    uint8_t write;
    uint8_t mode;
    int size;
    int num;
    char *srcfilepath;
    char *dstfilepath;
} fbackupMode;

typedef enum RESULT
{
    NO_ERROR = 0,
    PARAM_ERROR = -1,
    FILE_ERROR = -2,
    NO_MEMORY = -3,
} RESULT_T;

RESULT_T getParam(fbackupMode *fbackupmode, int argc, char **argv)
{
    char *param_ptr, *next_param_ptr;
    char choose;
    int param_size;
    //need to use next param, so loop from 0 - argc - 1 to avoid null point
    for (int i = 0; i < argc - 1; i++)
    {
        //acquire each param
        param_ptr = *(argv + i);
        //judge whether the first character of param is '-'
        if (*param_ptr == '-')
        {
            choose = *(param_ptr + 1);
            next_param_ptr = *(argv + i + 1);
            //printf("%c\n", choose);
            switch (choose)
            {
            case 'r':
                fbackupmode->source = 1;
                fbackupmode->srcfilepath = next_param_ptr;
                printf("source filepath : %s\n", fbackupmode->srcfilepath);
                break;
            case 'w':
                fbackupmode->write = 1;
                fbackupmode->dstfilepath = next_param_ptr;
                printf("destination filepath : %s\n", fbackupmode->dstfilepath);
                break;
            case 'm':
                fbackupmode->mode = atoi(next_param_ptr);
                printf("mode :%d\n", fbackupmode->mode);
                break;
            case 's':
                fbackupmode->size = atoi(next_param_ptr);
                //sizeof can't get point size so use function strlen
                char memory_size = *(next_param_ptr + strlen(next_param_ptr) - 1);
                if (memory_size == 'K' || memory_size == 'k')
                    fbackupmode->size *= 1024;
                else if (memory_size == 'M' || memory_size == 'm')
                    fbackupmode->size *= 1024 * 1024;
                printf("size :%d \n", fbackupmode->size);
                break;
            case 'n':
                fbackupmode->num = atoi(next_param_ptr);
                if (next_param_ptr)
                    printf("num :%d\n", fbackupmode->num);
                break;
            default:
                printf("sorry, no such param:-%c!\n", choose);
                return PARAM_ERROR;
                break;
            }
        }
    }
    return NO_ERROR;
}

RESULT_T checkParam(fbackupMode *fbackupmode)
{
    if (fbackupmode == NULL)
    {
        printf("fbackupmode is NULL\n");
        return PARAM_ERROR;
    }

    if (fbackupmode->write == 0)
    {
        printf("need to param:-w !");
        return PARAM_ERROR;
    }

    if (fbackupmode->mode > 2 || fbackupmode->mode < 0)
    {
        printf("mode should between 0 and 2");
        return PARAM_ERROR;
    }

    if (fbackupmode->mode == 2)
    {
        if (fbackupmode->num == 0 || fbackupmode->size == 0)
        {
            printf("need -n and -s parameters when in 2 mode");
            return PARAM_ERROR;
        }
    }

    return NO_ERROR;
}

/*****************************************************************
copy file from src to dst
size；buffer need to copy
remained_size: point to src file remained size
return: remained size after copy
******************************************************************/
void copyFile(FILE *src, FILE *dst, int size, int *remained_size)
{
    if (*remained_size < size)
        size = *remained_size;
    char *buffer = malloc(size);
    memset(buffer, 0, size);
    if (buffer == NULL)
    {
        printf("no memory!!!\n");
        return;
    }
    fwrite(buffer, size, 1, dst);
    free(buffer);
    fread(buffer, size, 1, src);
    *remained_size -= size;
    return;
}

RESULT_T processBackup(fbackupMode *fbackupmode)
{
    RESULT_T ret;
    FILE *fp_read = NULL, *fp_write = NULL;
    int file_size;
    struct stat statbuf;

    //read data
    if (fbackupmode->source)
    {
        fp_read = fopen(fbackupmode->srcfilepath, "r");
        if (fp_read == NULL)
        {
            printf("Fail to open source file:%s!\n", fbackupmode->srcfilepath);
            return FILE_ERROR;
        }
        //get file size
        stat(fbackupmode->srcfilepath, &statbuf);
        file_size = statbuf.st_size;
    }

    switch (fbackupmode->mode)
    {
    case 0:
    {
        fp_write = fopen(fbackupmode->dstfilepath, "w");
        if (fp_write != NULL)
        {
            if (fbackupmode->size)
            {
                copyFile(fp_read, fp_write, fbackupmode->size, &file_size);
            }
            else
            {
                copyFile(fp_read, fp_write, file_size, &file_size);
            }
        }
        else
        {
            if (fp_read != NULL)
                fclose(fp_read);
            return FILE_ERROR;
        }
        fclose(fp_write);
    }
    break;

    case 1:
    {
        fp_write = fopen(fbackupmode->dstfilepath, "w");
        if (fp_write != NULL)
        {
            if (fbackupmode->size)
            {
                while (file_size > 0)
                {
                    copyFile(fp_read, fp_write, fbackupmode->size, &file_size);
                    fseek(fp_write, 0, SEEK_SET);
                }
            }
            else
            {
                copyFile(fp_read, fp_write, file_size, &file_size);
            }
        }
        else
        {
            if (fp_read != NULL)
                fclose(fp_read);
            return FILE_ERROR;
        }
        fclose(fp_write);
    }
    break;

    case 2:
    {
        char file_path[128] = {0};
        strcpy(file_path, fbackupmode->dstfilepath);
        int file_maxnum = strlen(fbackupmode->dstfilepath);
        int file_num = 0;

        do
        {
            //add file num after file path
            //linux doesn't support itoa
            //itoa(file_num, &file_path[file_maxnum], 10);
            sprintf(&file_path[file_maxnum], "%d", file_num);
            printf("now write file path is %s\n", file_path);
            fp_write = fopen(file_path, "w");
            if (fp_write == NULL)
            {
                if (fp_read != NULL)
                    fclose(fp_read);
                return FILE_ERROR;
            }

            copyFile(fp_read, fp_write, fbackupmode->size, &file_size);

            if (fp_write != NULL)
                fclose(fp_write);

            if (file_num >= (fbackupmode->num - 1))
                file_num = 0;
            else
                file_num++;
        } while (file_size > 0);
    }
    break;

    default:
        return PARAM_ERROR;
        break;
    }

    //release all file and memory
    if (fp_read != NULL)
        fclose(fp_read);

    return ret;
}

int main(int argc, char **argv)
{
    RESULT_T ret = 0;
    fbackupMode fbackup_mode = {0};

    ret = getParam(&fbackup_mode, argc, argv);
    if (ret != NO_ERROR)
        return ret;

    ret = checkParam(&fbackup_mode);
    if (ret != NO_ERROR)
        return ret;

    ret = processBackup(&fbackup_mode);
    return ret;
}