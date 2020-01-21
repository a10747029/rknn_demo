#include "algorithm.h"
#include <execinfo.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "tjstal.h"


#define MODEL_NAME    "/usr/share/smart_camera/ssd_inception_v2.rknn"

#define BOX_PRIORS_TXT_PATH "/usr/share/smart_camera/box_priors.txt"
#define LABEL_NAME_TXT_PATH "/usr/share/smart_camera/coco_labels_list.txt"
#define MAX_OUTPUT_NUM 100

float MIN_SCORE = 0.4f;
float NMS_THRESHOLD = 0.45f;

char *labels[91];
float box_priors[4][NUM_RESULTS];

rknn_context ctx;
void dump(int signo)
{
        void *array[10];
        size_t size;
        char **strings;
        size_t i;

        size = backtrace (array, 10);
        strings = backtrace_symbols (array, size);

        printf ("Obtained %zd stack frames.\n", size);

        for (i = 0; i < size; i++)
                printf ("%s\n", strings[i]);

        free (strings);

        exit(0);
}
int max(int a, int b)
{
    if(a>b) return a;
    else    return b;
}
char * readLine(FILE *fp, char *buffer, int *len)
{
    int ch;
    int i = 0;
    size_t buff_len = 0;

    buffer = malloc(buff_len + 1);
    if (!buffer) return NULL;  // Out of memory

    while ((ch = fgetc(fp)) != '\n' && ch != EOF)
    {
        buff_len++;
        void *tmp = realloc(buffer, buff_len + 1);
        if (tmp == NULL)
        {
            free(buffer);
            return NULL; // Out of memory
        }
        buffer = tmp;

        buffer[i] = (char) ch;
        i++;
    }
    buffer[i] = '\0';

        *len = buff_len;

    // Detect end
    if (ch == EOF && (i == 0 || ferror(fp)))
    {
        free(buffer);
        return NULL;
    }
    return buffer;
}

int readLines(const char *fileName, char *lines[]){
        FILE* file = fopen(fileName, "r");
        char *s;
        int i = 0;
        int n = 0;
        while ((s = readLine(file, s, &n)) != NULL) {
                lines[i++] = s;
        }
        return i;
}

int loadLabelName(const char* locationFilename, char* labels[]) {
        readLines(locationFilename, labels);
    return 0;
}
int loadBoxPriors(const char* locationFilename, float (*boxPriors)[NUM_RESULTS])
{
    const char *d = " ";
        char *lines[4];
        int count = readLines(locationFilename, lines);
        // printf("line count %d\n", count);
        // for (int i = 0; i < count; i++) {
                // printf("%s\n", lines[i]);
        // }
    for (int i = 0; i < 4; i++)
    {
        char *line_str = lines[i];
        char *p;
        p = strtok(line_str, d);
        int priorIndex = 0;
        while (p) {
            float number = (float)(atof(p));
            boxPriors[i][priorIndex++] = number;
            p=strtok(NULL, d);
        }
        if (priorIndex != NUM_RESULTS) {
                        printf("error\n");
            return -1;
        }
    }
    return 0;
}

float CalculateOverlap(float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1, float ymax1) {
    float w = fmax(0.f, fmin(xmax0, xmax1) - fmax(xmin0, xmin1));
    float h = fmax(0.f, fmin(ymax0, ymax1) - fmax(ymin0, ymin1));
    float i = w * h;
    float u = (xmax0 - xmin0) * (ymax0 - ymin0) + (xmax1 - xmin1) * (ymax1 - ymin1) - i;
    return u <= 0.f ? 0.f : (i / u);
}

float expit(float x) {
    return (float) (1.0 / (1.0 + expf(-x)));
}

void decodeCenterSizeBoxes(float* predictions, float (*boxPriors)[NUM_RESULTS]) {

    for (int i = 0; i < NUM_RESULTS; ++i) {
        float ycenter = predictions[i*4+0] / Y_SCALE * boxPriors[2][i] + boxPriors[0][i];
        float xcenter = predictions[i*4+1] / X_SCALE * boxPriors[3][i] + boxPriors[1][i];
        float h = (float) expf(predictions[i*4 + 2] / H_SCALE) * boxPriors[2][i];
        float w = (float) expf(predictions[i*4 + 3] / W_SCALE) * boxPriors[3][i];

        float ymin = ycenter - h / 2.0f;
        float xmin = xcenter - w / 2.0f;
        float ymax = ycenter + h / 2.0f;
        float xmax = xcenter + w / 2.0f;

        predictions[i*4 + 0] = ymin;
        predictions[i*4 + 1] = xmin;
        predictions[i*4 + 2] = ymax;
        predictions[i*4 + 3] = xmax;
    }
}
int filterValidResult(float * outputClasses, int (*output)[NUM_RESULTS], int numClasses)
{
    int validCount = 0;
    // Scale them back to the input size.
    for (int i = 0; i < NUM_RESULTS; ++i) {
        float topClassScore = (float)(-1000.0);
        int topClassScoreIndex = -1;

        // Skip the first catch-all class.
        for (int j = 1; j < numClasses; ++j) {
            float score = expit(outputClasses[i*numClasses+j]);
            if (score > topClassScore) {
                topClassScoreIndex = j;
                topClassScore = score;
            }
        }

        if (topClassScore >= MIN_SCORE) {
            output[0][validCount] = i;
            output[1][validCount] = topClassScoreIndex;
            ++validCount;
        }
    }

    return validCount;
}
int nms(int validCount, float* outputLocations, int (*output)[NUM_RESULTS])
{
    for (int i=0; i < validCount; ++i) {
        if (output[0][i] == -1) {
            continue;
        }
        int n = output[0][i];
        for (int j=i + 1; j<validCount; ++j) {
            int m = output[0][j];
            if (m == -1) {
                continue;
            }
            float xmin0 = outputLocations[n*4 + 1];
            float ymin0 = outputLocations[n*4 + 0];
            float xmax0 = outputLocations[n*4 + 3];
            float ymax0 = outputLocations[n*4 + 2];

            float xmin1 = outputLocations[m*4 + 1];
            float ymin1 = outputLocations[m*4 + 0];
            float xmax1 = outputLocations[m*4 + 3];
            float ymax1 = outputLocations[m*4 + 2];

            float iou = CalculateOverlap(xmin0, ymin0, xmax0, ymax0, xmin1, ymin1, xmax1, ymax1);

            if (iou >= NMS_THRESHOLD) {
                output[0][j] = -1;
            }
        }
    }
}

int postProcessSSD(float * predictions, float *output_classes, int width,
                   int heigh)
{
    int output[2][NUM_RESULTS];
    decodeCenterSizeBoxes(predictions, box_priors);
    unsigned char alertData = 0;

    int validCount = filterValidResult(output_classes, output, NUM_CLASS);

    if (validCount > 100) {
        printf("validCount too much !!\n");
        return -1;
    }

    /* detect nest box */
    nms(validCount, predictions, output);

    int last_count = 0;
    int maxTrackLifetime = 3;
    int track_num_input = 0;
    for (int i = 0; i < validCount; ++i) {
            if (output[0][i] == -1) {
            continue;
        }
        int n = output[0][i];
        int topClassScoreIndex = output[1][i];
        
        alertData = 0;

        int x1 = (int)(predictions[n * 4 + 1] * width);
        int y1 = (int)(predictions[n * 4 + 0] * heigh);
        int x2 = (int)(predictions[n * 4 + 3] * width);
        int y2 = (int)(predictions[n * 4 + 2] * heigh);
	x1 = max(x1,0);
        y1 = max(y1,0);
        x2 = max(x2,0);
        y2 = max(y2,0);
        // There are a bug show toothbrush always
        if (x1 == 0 && x2 == 0 && y1 == 0 && y2 == 0)
            continue;
        printf("%s\t@ (%d, %d, %d, %d)\n", labels[topClassScoreIndex], x1, y1, x2, y2);
        alertData = AlertJudge(x1,y1,x2,y2);
        UpldAlertDataForCustomer(alertData,0x01,"testmultiMedia",x1,y1,x2,y2);


    }

}

unsigned char *load_model(const char *filename, int *model_size)
{
    FILE *fp = fopen(filename, "rb");

    if(fp == NULL) {
        printf("fopen %s fail!\n", filename);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    int model_len = ftell(fp);
    unsigned char *model = (unsigned char*)malloc(model_len);
    fseek(fp, 0, SEEK_SET);
    if(model_len != fread(model, 1, model_len, fp)) {
        printf("fread %s fail!\n", filename);
        free(model);
        return NULL;
    }
    *model_size = model_len;
    if(fp) {
        fclose(fp);
    }
    return model;
}

int ssd_init()
{
    int status = 0;
    int ret = 0;
    int model_len = 0;
    unsigned char* model;

    model = load_model(MODEL_NAME, &model_len);

    printf("loadLabelName\n");
    ret = loadLabelName(LABEL_NAME_TXT_PATH, labels);
    if (ret < 0) {
	return -1;
    }
    for (int i = 0; i < NUM_CLASS; i++)
	printf("%s\n", labels[i]);


    status = rknn_init(&ctx, model, model_len, 0);
    if(status < 0) {
        printf("rknn_init fail! ret=%d\n", status);
        return -1;
    }

    // Get Model Input Output Info
    rknn_input_output_num io_num;
    status = rknn_query(ctx, RKNN_QUERY_IN_OUT_NUM, &io_num, sizeof(io_num));
    if (status != RKNN_SUCC) {
        printf("rknn_query fail! ret=%d\n", status);
        return -1;
    }
    printf("model input num: %d, output num: %d\n", io_num.n_input, io_num.n_output);

    printf("input tensors:\n");
    rknn_tensor_attr input_attrs[io_num.n_input];
    memset(input_attrs, 0, sizeof(input_attrs));
    for (int i = 0; i < io_num.n_input; i++) {
        input_attrs[i].index = i;
        status = rknn_query(ctx, RKNN_QUERY_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr));
        if (status != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", status);
            return -1;
        }
       //print_rknn_tensor(&(input_attrs[i]));
    }

    printf("output tensors:\n");
    rknn_tensor_attr output_attrs[io_num.n_output];
    memset(output_attrs, 0, sizeof(output_attrs));
    for (int i = 0; i < io_num.n_output; i++) {
        output_attrs[i].index = i;
        status = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr));
        if (status != RKNN_SUCC) {
            printf("rknn_query fail! ret=%d\n", status);
            return -1;
        }
        //print_rknn_tensor(&(output_attrs[i]));
    }

    ret = loadBoxPriors(BOX_PRIORS_TXT_PATH, box_priors);
    if (ret < 0) {
	return -1;
    }

}

int run_ssd(char* in_data)
{
    int status = 0;
    int in_size;
    int out_size0;
    int out_size1;
    int w = 300;
    int h = 300;
    int c = 24;
    float *out_data0 = NULL;
    float *out_data1 = NULL;
  //   printf("camera callback w=%d h=%d c=%d\n", w, h, c);

    long runTime1 = getCurrentTime();

    long setinputTime1 = getCurrentTime();
    // Set Input Data
    rknn_input inputs[1];
    memset(inputs, 0, sizeof(inputs));
    inputs[0].index = 0;
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].size = w*h*c/8;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    inputs[0].buf = in_data;

    status = rknn_inputs_set(ctx, 1, inputs);
    if(status < 0) {
        printf("rknn_input_set fail! ret=%d\n", status);
        return -1;
    }
    long setinputTime2 = getCurrentTime();

    // printf("set input time:%0.2ldms\n", setinputTime2-setinputTime1);

    status = rknn_run(ctx, NULL);
    if(status < 0) {
        printf("rknn_run fail! ret=%d\n", status);
        return -1;
    }

    // Get Output
    rknn_output outputs[2];
    memset(outputs, 0, sizeof(outputs));
    outputs[0].want_float = 1;
    outputs[1].want_float = 1;
    status = rknn_outputs_get(ctx, 2, outputs, NULL);
    if(status < 0) {
        printf("rknn_outputs_get fail! ret=%d\n", status);
        return -1;
    }

    int out0_elem_num = NUM_RESULTS * NUM_CLASS;
    int out1_elem_num = NUM_RESULTS * 4;

    float *output0 = malloc(out0_elem_num*sizeof(float));
    float *output1 = malloc(out1_elem_num*sizeof(float));

    memcpy(output0, outputs[1].buf, out0_elem_num*sizeof(float));
    memcpy(output1, outputs[0].buf, out1_elem_num*sizeof(float));

    rknn_outputs_release(ctx, 2, outputs);
    postProcessSSD(output1, output0, w, h);
    return 0;
}

unsigned char AlertJudge(int x0,int y0,int x1,int y1)
{
    if(LINE1(x0) < y1) {
        return 3;
    }
    else if(LINE2(x0) < y1) {
        return 2;
    }
    else if(LINE3(x0) < y1) {
        return 1;
    }
    else if(LINE3(x0) >= y1) {
        return 0;
    }

    /*if(y1 > 178 && LINE1(x0) < y1) {
        return 3;
    }
    else if(y1 > 178 && LINE2(x0) < y1) {
        return 2;
    }
    else if(y1 > 178 && LINE3(x0) < y1) {
        return 1;
    }
    else if(y1 < 178 || LINE3(x0) >= y1) {
        return 0;
    */
}

