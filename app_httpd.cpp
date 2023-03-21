#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "camera_index.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "fd_forward.h"
#include "fr_forward.h"
#define ENROLL_CONFIRM_TIMES 5
#define FACE_ID_SAVE_NUMBER 7
#define COLOR_WHITE  0x00FFFFFF
#define COLOR_BLACK  0x00000000
#define COLOR_RED    0x000000FF
#define COLOR_GREEN  0x0000FF00
#define COLOR_BLUE   0x00FF0000
#define COLOR_YELLOW (COLOR_RED | COLOR_GREEN)
#define COLOR_CYAN   (COLOR_BLUE | COLOR_GREEN)
#define COLOR_PURPLE (COLOR_BLUE | COLOR_RED)
#define FLASH_LED 4
typedef struct {
        size_t size; //number of values used for filtering
        size_t index; //current index value
        size_t count; //number of value 
        int sum;
        int * values; //array with values
} ra_filter_t;

typedef struct {
        httpd_req_t *reqst;
        size_t len;
} jpg_chunking_t;

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static ra_filter_t ra_filter;
httpd_handle_t stream_httpd = NULL;
httpd_handle_t camera_httpd = NULL;

static mtmn_config_t mtmn_config = {0};
static int8_t detection_on = 1;
static int8_t recognition_on = 0;
static int8_t is_enrolling = 0;
static face_id_list id_list = {0};
bool fden=false;
String cName = "";
String names[10]={};


static void drawing_face_boxes(dl_matrix3du_t *img_matrix, box_array_t *boxes, int face_id){
    int x, y, w, h, i;
    uint32_t color = COLOR_YELLOW;
    if(face_id < 0){
        color = COLOR_RED;
    } else if(face_id > 0){
        color = COLOR_GREEN;
    }
    fb_data_t fb;
    fb.width = img_matrix->w;
    fb.height = img_matrix->h;
    fb.data = img_matrix->item;
    fb.bytes_per_pixel = 3;
    fb.format = FB_BGR888;
    for (i = 0; i < boxes->len; i++){
        // rectangle box
        x = (int)boxes->box[i].box_p[0];
        y = (int)boxes->box[i].box_p[1];
        w = (int)boxes->box[i].box_p[2] - x + 1;
        h = (int)boxes->box[i].box_p[3] - y + 1;
        fb_gfx_drawFastHLine(&fb, x, y, w, color);
        fb_gfx_drawFastHLine(&fb, x, y+h-1, w, color);
        fb_gfx_drawFastVLine(&fb, x, y, h, color);
        fb_gfx_drawFastVLine(&fb, x+w-1, y, h, color);
#if 0
        // landmark
        int x0, y0, j;
        for (j = 0; j < 10; j+=2) {
            x0 = (int)boxes->landmark[i].landmark_p[j];
            y0 = (int)boxes->landmark[i].landmark_p[j+1];
            fb_gfx_fillRect(&fb, x0, y0, 3, 3, color);
        }
#endif
    }
}

static ra_filter_t * ra_filter_init(ra_filter_t * filter, size_t sample_size){
    memset(filter, 0, sizeof(ra_filter_t));

    filter->values = (int *)malloc(sample_size * sizeof(int));
    if(!filter->values){
        return NULL;
    }
    memset(filter->values, 0, sample_size * sizeof(int));

    filter->size = sample_size;
    return filter;
}
static void rgb_print(dl_matrix3du_t *img_matrix, uint32_t color, const char * str){
    fb_data_t fb;
    fb.width = img_matrix->w;
    fb.height = img_matrix->h;
    fb.data = img_matrix->item;
    fb.bytes_per_pixel = 3;
    fb.format = FB_BGR888;
    fb_gfx_print(&fb, (fb.width - (strlen(str) * 14)) / 2, 10, color, str);
}
static int rgb_printf(dl_matrix3du_t *img_matrix, uint32_t color, const char *format, ...){
    char loc_buf[64];
    char * temp = loc_buf;
    int len;
    va_list arg;
    va_list copy;
    va_start(arg, format);
    va_copy(copy, arg);
    len = vsnprintf(loc_buf, sizeof(loc_buf), format, arg);
    va_end(copy);
    if(len >= sizeof(loc_buf)){
        temp = (char*)malloc(len+1);
        if(temp == NULL) {
            return 0;
        }
    }
    vsnprintf(temp, len+1, format, arg);
    va_end(arg);
    rgb_print(img_matrix, color, temp);
    if(len > 64){
        free(temp);
    }
    return len;
}

static int running_face_recognition(dl_matrix3du_t *img_matrix, box_array_t *net_boxes, httpd_req_t *reqst){
    dl_matrix3du_t *aligned_face = NULL;
    int matched_id = 0;

    aligned_face = dl_matrix3du_alloc(1, FACE_WIDTH, FACE_HEIGHT, 3);
    if(!aligned_face){
        Serial.println("Face recognition buffer allocation failed");
        return matched_id;
    }
    if (align_face(net_boxes, img_matrix, aligned_face) == ESP_OK){
        if (is_enrolling == 1){
            int8_t left_sample_face = enroll_face(&id_list, aligned_face);
            rgb_printf(img_matrix, COLOR_CYAN, "ID[%u] Sample[%u]", id_list.tail, ENROLL_CONFIRM_TIMES - left_sample_face);
            if (left_sample_face == 0){
                is_enrolling = 0;
                names[id_list.tail-1]=cName;
                detection_on = 0;
                recognition_on = 0;
            }
          
        } else {
            matched_id = recognize_face(&id_list, aligned_face);
            if (matched_id >= 0) {
                rgb_printf(img_matrix, COLOR_GREEN, "%s", names[matched_id]);
            } else {  
                rgb_print(img_matrix, COLOR_YELLOW, "Unknown Face!");
                matched_id = -1;
            }
        }
    } 
    dl_matrix3du_free(aligned_face);
    return matched_id;
}

static size_t jpg_encode_stream(void * arg, size_t index, const void* data, size_t len){
    jpg_chunking_t *j = (jpg_chunking_t *)arg;
    if(!index){
        j->len = 0;
    }
    if(httpd_resp_send_chunk(j->reqst, (const char *)data, len) != ESP_OK){
        return 0;
    }
    j->len += len;
    return len;
}
static int ra_filter_run(ra_filter_t * filter, int value){
    if(!filter->values){
        return value;
    }
    filter->sum -= filter->values[filter->index];
    filter->values[filter->index] = value;
    filter->sum += filter->values[filter->index];
    filter->index++;
    filter->index = filter->index % filter->size;
    if (filter->count < filter->size) {
        filter->count++;
    }
    return filter->sum / filter->count;
}

static esp_err_t video_handler(httpd_req_t *reqst){
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[64];
    dl_matrix3du_t *img_matrix = NULL;
    bool detected = false;
    int face_id = 0;
    int64_t frm_start = 0;
    int64_t frm_ready = 0;
    int64_t frm_face = 0;
    int64_t frm_recognize = 0;
    int64_t frm_encode = 0;

    static int64_t last_frame = 0;
    if(!last_frame) {
        last_frame = esp_timer_get_time();
    }

    res = httpd_resp_set_type(reqst, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK){
        return res;
    }

    httpd_resp_set_hdr(reqst, "Access-Control-Allow-Origin", "*");

    while(true){
        detected = false;
        face_id = 0;
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            res = ESP_FAIL;
        } else {
            frm_start = esp_timer_get_time();
            frm_ready = frm_start;
            frm_face = frm_start;
            frm_encode = frm_start;
            frm_recognize = frm_start;
            if(!detection_on || fb->width > 400){
                if(fb->format != PIXFORMAT_JPEG){
                    bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
                    esp_camera_fb_return(fb);
                    fb = NULL;
                    if(!jpeg_converted){
                        Serial.println("JPEG compression failed");
                        res = ESP_FAIL;
                    }
                } else {
                    _jpg_buf_len = fb->len;
                    _jpg_buf = fb->buf;
                }
            } else {
                 img_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);

                if (!img_matrix) {
                    Serial.println("dl_matrix3du_alloc failed");
                    res = ESP_FAIL;
                } else {
                    if(!fmt2rgb888(fb->buf, fb->len, fb->format, img_matrix->item)){
                        Serial.println("fmt2rgb888 failed");
                        res = ESP_FAIL;
                    } else {
                        frm_ready = esp_timer_get_time();
                        box_array_t *net_boxes = nullptr; 
                        if(detection_on){
                            net_boxes = face_detect(img_matrix, &mtmn_config);
                        }
                        frm_face = esp_timer_get_time();
                        frm_recognize = frm_face;
                        if (net_boxes || fb->format != PIXFORMAT_JPEG){
                            if(net_boxes){
                                detected = true;
                                if(recognition_on){
                                    face_id = running_face_recognition(img_matrix, net_boxes,reqst);
                                }
                                frm_recognize = esp_timer_get_time();
                               
                                drawing_face_boxes(img_matrix, net_boxes, face_id);                            
              
                            }
                            if(!fmt2jpg(img_matrix->item, fb->width*fb->height*3, fb->width, fb->height, PIXFORMAT_RGB888, 90, &_jpg_buf, &_jpg_buf_len)){
                                Serial.println("fmt2jpg failed");
                                res = ESP_FAIL;
                            }
                            esp_camera_fb_return(fb);
                            fb = NULL;
                        } else {
                            _jpg_buf = fb->buf;
                            _jpg_buf_len = fb->len;
                        }
                        frm_encode = esp_timer_get_time();
                    }
                    dl_matrix3du_free(img_matrix);
                }
            }
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(reqst, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(res == ESP_OK){
            size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(reqst, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK){
            res = httpd_resp_send_chunk(reqst, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(fb){
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        } else if(_jpg_buf){
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if(res != ESP_OK){
            break;
        }
        int64_t fr_end = esp_timer_get_time();

        int64_t ready_time = (frm_ready - frm_start)/1000;
        int64_t face_time = (frm_face - frm_ready)/1000;
        int64_t recognize_time = (frm_recognize - frm_face)/1000;
        int64_t encode_time = (frm_encode - frm_recognize)/1000;
        int64_t process_time = (frm_encode - frm_start)/1000;
        
        int64_t frame_time = fr_end - last_frame;
        last_frame = fr_end;
        frame_time /= 1000;
        uint32_t avg_frame_time = ra_filter_run(&ra_filter, frame_time);

    }

    last_frame = 0;
    return res;
}

static esp_err_t cmd_handler(httpd_req_t *reqst){
    char*  buf;
    size_t buf_len;
    char var_value[32] = {0,};
    char value[32] = {0,};

    buf_len = httpd_req_get_url_query_len(reqst) + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if(!buf){
            httpd_resp_send_500(reqst);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(reqst, buf, buf_len) == ESP_OK) {
            if (httpd_query_key_value(buf, "var", var_value, sizeof(var_value)) == ESP_OK &&
                httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK) {
            } else {
                free(buf);
                httpd_resp_send_404(reqst);
                return ESP_FAIL;
            }
        } else {
            free(buf);
            httpd_resp_send_404(reqst);
            return ESP_FAIL;
        }
        free(buf);
    } else {
        httpd_resp_send_404(reqst);
        return ESP_FAIL;
    }

    int val = atoi(value);
    sensor_t * s = esp_camera_sensor_get();
    int res = 0;

    if(!strcmp(var_value, "framesize")) {
        if(s->pixformat == PIXFORMAT_JPEG) res = s->set_framesize(s, (framesize_t)val);
    }
    else if(!strcmp(var_value, "quality")) res = s->set_quality(s, val);
    else if(!strcmp(var_value, "contrast")) res = s->set_contrast(s, val);
    else if(!strcmp(var_value, "brightness")) res = s->set_brightness(s, val);
    else if(!strcmp(var_value, "saturation")) res = s->set_saturation(s, val);
    else if(!strcmp(var_value, "gainceiling")) res = s->set_gainceiling(s, (gainceiling_t)val);
    else if(!strcmp(var_value, "colorbar")) res = s->set_colorbar(s, val);
    else if(!strcmp(var_value, "awb")) res = s->set_whitebal(s, val);
    else if(!strcmp(var_value, "agc")) res = s->set_gain_ctrl(s, val);
    else if(!strcmp(var_value, "aec")) res = s->set_exposure_ctrl(s, val);
    else if(!strcmp(var_value, "hmirror")) res = s->set_hmirror(s, val);
    else if(!strcmp(var_value, "vflip")) res = s->set_vflip(s, val);
    else if(!strcmp(var_value, "awb_gain")) res = s->set_awb_gain(s, val);
    else if(!strcmp(var_value, "agc_gain")) res = s->set_agc_gain(s, val);
    else if(!strcmp(var_value, "aec_value")) res = s->set_aec_value(s, val);
    else if(!strcmp(var_value, "aec2")) res = s->set_aec2(s, val);
    else if(!strcmp(var_value, "dcw")) res = s->set_dcw(s, val);
    else if(!strcmp(var_value, "bpc")) res = s->set_bpc(s, val);
    else if(!strcmp(var_value, "wpc")) res = s->set_wpc(s, val);
    else if(!strcmp(var_value, "raw_gma")) res = s->set_raw_gma(s, val);
    else if(!strcmp(var_value, "lenc")) res = s->set_lenc(s, val);
    else if(!strcmp(var_value, "special_effect")) res = s->set_special_effect(s, val);
    else if(!strcmp(var_value, "wb_mode")) res = s->set_wb_mode(s, val);
    else if(!strcmp(var_value, "ae_level")) res = s->set_ae_level(s, val);
    else if(!strcmp(var_value, "face_detect")) {
        detection_on = val;
         if(!detection_on) {
             fden=false;
        }
        else{fden=true;}
       
        if(!detection_on) {
            recognition_on = 0;
        }
    }
    else if(!strcmp(var_value, "face_enroll")) is_enrolling = val;
    else if(!strcmp(var_value, "face_recognize")) {
        recognition_on = val;
        if(recognition_on){
            detection_on = val;
        }
    }
    else {
        res = -1;
    }

    if(res){
        return httpd_resp_send_500(reqst);
    }

    httpd_resp_set_hdr(reqst, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(reqst, NULL, 0);
}


static esp_err_t index_handler(httpd_req_t *reqst){
    httpd_resp_set_type(reqst, "text/html");
    httpd_resp_set_hdr(reqst, "Content-Encoding", "gzip");
    sensor_t * s = esp_camera_sensor_get();
    if (s->id.PID == OV3660_PID) {
        return httpd_resp_send(reqst, (const char *)index_ov3660_html_gz, index_ov3660_html_gz_len);
    }
    return httpd_resp_send(reqst, (const char *)index_ov2640_html_gz, index_ov2640_html_gz_len);
}

static esp_err_t led_handler(httpd_req_t *reqst){
    char*  buf;
    size_t buf_len;
    char var_value[32] = {0,};
    char value[32] = {0,};

    buf_len = httpd_req_get_url_query_len(reqst) + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if(!buf){
            httpd_resp_send_500(reqst);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(reqst, buf, buf_len) == ESP_OK) {
            if (httpd_query_key_value(buf, "var", var_value, sizeof(var_value)) == ESP_OK &&
                httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK) {
            } else {
                free(buf);
                httpd_resp_send_404(reqst);
                return ESP_FAIL;
            }
        } else {
            free(buf);
            httpd_resp_send_404(reqst);
            return ESP_FAIL;
        }
        free(buf);
    } else {
        httpd_resp_send_404(reqst);
        return ESP_FAIL;
    }

    int val = atoi(value);
    sensor_t * s = esp_camera_sensor_get();
    int res = 0;


    if(!strcmp(var_value, "flash")) {
      if(val){
        pinMode(FLASH_LED, OUTPUT);
        digitalWrite(FLASH_LED, HIGH);  
      }
      else{
        pinMode(FLASH_LED, OUTPUT);
        digitalWrite(FLASH_LED, LOW);  
      }
    }
    else {
        res = -1;
    }

    if(res){
        return httpd_resp_send_500(reqst);
    }

    httpd_resp_set_hdr(reqst, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(reqst, "flashon", 20);    
}

static esp_err_t detect_handler(httpd_req_t *reqst){
    char*  buf;
    size_t buf_len;
    char var_value[32] = {0,};
    char value[32] = {0,};

    buf_len = httpd_req_get_url_query_len(reqst) + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if(!buf){
            httpd_resp_send_500(reqst);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(reqst, buf, buf_len) == ESP_OK) {
            if (httpd_query_key_value(buf, "var", var_value, sizeof(var_value)) == ESP_OK &&
                httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK) {
            } else {
                free(buf);
                httpd_resp_send_404(reqst);
                return ESP_FAIL;
            }
        } else {
            free(buf);
            httpd_resp_send_404(reqst);
            return ESP_FAIL;
        }
        free(buf);
    } else {
        httpd_resp_send_404(reqst);
        return ESP_FAIL;
    }

    int val = atoi(value);
    sensor_t * s = esp_camera_sensor_get();
    int res = 0;


    if(!strcmp(var_value, "D1")) {
      if(val){
      detection_on = 1;
      recognition_on = 0;
      is_enrolling = 0;  
      }
      else{
        detection_on = 0; 
        recognition_on = 0;
        is_enrolling = 0; 
        
      }
    }
    else {
        res = -1;
    }

    if(res){
        return httpd_resp_send_500(reqst);
    }

    httpd_resp_set_hdr(reqst, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(reqst, NULL, 0);    
}
static esp_err_t enroll_handler(httpd_req_t *reqst){
    char*  buf;
    size_t buf_len;
    char var_value[32] = {0,};
    char value[32] = {0,};

    buf_len = httpd_req_get_url_query_len(reqst) + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if(!buf){
            httpd_resp_send_500(reqst);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(reqst, buf, buf_len) == ESP_OK) {
            if (httpd_query_key_value(buf, "var", var_value, sizeof(var_value)) == ESP_OK &&
                httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK) {
            } else {
                free(buf);
                httpd_resp_send_404(reqst);
                return ESP_FAIL;
            }
        } else {
            free(buf);
            httpd_resp_send_404(reqst);
            return ESP_FAIL;
        }
        free(buf);
    } else {
        httpd_resp_send_404(reqst);
        return ESP_FAIL;
    }

    int val = atoi(value);
    sensor_t * s = esp_camera_sensor_get();
    int res = 0;
    
    cName=String(var_value);
     if(cName!="") { 
      if(val)
      {
      detection_on = 1;
      recognition_on = 1;
      is_enrolling = 1; 
      cName=String(var_value);
      }
      else{
           detection_on = 0;
          recognition_on = 0;
          is_enrolling = 0; 
      }
    }
    else {
        res = -1;
    }

    if(res){
        return httpd_resp_send_500(reqst);
    }

    httpd_resp_set_hdr(reqst, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(reqst, NULL, 0);    
    //return 0;
}

static esp_err_t recog_handler(httpd_req_t *reqst){
    char*  buf;
    size_t buf_len;
    char var_value[32] = {0,};
    char value[32] = {0,};

    buf_len = httpd_req_get_url_query_len(reqst) + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if(!buf){
            httpd_resp_send_500(reqst);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(reqst, buf, buf_len) == ESP_OK) {
            if (httpd_query_key_value(buf, "var", var_value, sizeof(var_value)) == ESP_OK &&
                httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK) {
            } else {
                free(buf);
                httpd_resp_send_404(reqst);
                return ESP_FAIL;
            }
        } else {
            free(buf);
            httpd_resp_send_404(reqst);
            return ESP_FAIL;
        }
        free(buf);
    } else {
        httpd_resp_send_404(reqst);
        return ESP_FAIL;
    }

    int val = atoi(value);
    sensor_t * s = esp_camera_sensor_get();
    int res = 0;


    if(!strcmp(var_value, "frecog")) {
      if(val){
      detection_on = 1;
      recognition_on = 1;
      is_enrolling = 0;  
      }
      else{
          detection_on = 0;
          recognition_on = 0;
          is_enrolling = 0; 
      }
    }
    else {
        res = -1;
    }

    if(res){
        return httpd_resp_send_500(reqst);
    }

    httpd_resp_set_hdr(reqst, "Access-Control-Allow-Origin", "*");
    return httpd_resp_send(reqst, NULL, 0);    
}
static esp_err_t rc_handler(httpd_req_t *reqst){
    char*  buf;
    size_t buf_len;
    char var_value[32] = {0,};
    char value[32] = {0,};

    buf_len = httpd_req_get_url_query_len(reqst) + 1;
    if (buf_len > 1) {
        buf = (char*)malloc(buf_len);
        if(!buf){
            httpd_resp_send_500(reqst);
            return ESP_FAIL;
        }
        if (httpd_req_get_url_query_str(reqst, buf, buf_len) == ESP_OK) {
            if (httpd_query_key_value(buf, "var", var_value, sizeof(var_value)) == ESP_OK &&
                httpd_query_key_value(buf, "val", value, sizeof(value)) == ESP_OK) {
            } else {
                free(buf);
                httpd_resp_send_404(reqst);
                return ESP_FAIL;
            }
        } else {
            free(buf);
            httpd_resp_send_404(reqst);
            return ESP_FAIL;
        }
        free(buf);
    } else {
        httpd_resp_send_404(reqst);
        return ESP_FAIL;
    }

    int val = atoi(value);
    sensor_t * s = esp_camera_sensor_get();
    int res = 0;


    if(!strcmp(var_value, "rc")) {
   
    }
    else {
        res = -1;
    }

    if(res){
        return httpd_resp_send_500(reqst);
    }
    httpd_resp_set_hdr(reqst, "Access-Control-Allow-Origin", "*");
    
    if(is_enrolling){
       return httpd_resp_send(reqst, "ennc" , 5); 
    }
    else{
      return httpd_resp_send(reqst, "enc" , 5); 
    }
       
}

void startCameraServer(){
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    httpd_uri_t index_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = index_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t cmd_uri = {
        .uri       = "/control",
        .method    = HTTP_GET,
        .handler   = cmd_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t led_uri = {
        .uri       = "/led",
        .method    = HTTP_GET,
        .handler   = led_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t detect_uri = {
        .uri       = "/detect",
        .method    = HTTP_GET,
        .handler   = detect_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t enroll_uri = {
        .uri       = "/enroll",
        .method    = HTTP_GET,
        .handler   = enroll_handler,
        .user_ctx  = NULL
    };
     httpd_uri_t rc_uri = {
        .uri       = "/rc",
        .method    = HTTP_GET,
        .handler   = rc_handler,
        .user_ctx  = NULL
    };
    httpd_uri_t recog_uri = {
        .uri       = "/recog",
        .method    = HTTP_GET,
        .handler   = recog_handler,
        .user_ctx  = NULL
    };

   httpd_uri_t stream_uri = {
        .uri       = "/stream",
        .method    = HTTP_GET,
        .handler   = video_handler,
        .user_ctx  = NULL
    };
    ra_filter_init(&ra_filter, 20);
    mtmn_config.type = FAST;
    mtmn_config.min_face = 80;
    mtmn_config.pyramid = 0.707;
    mtmn_config.pyramid_times = 4;
    mtmn_config.p_threshold.score = 0.6;
    mtmn_config.p_threshold.nms = 0.7;
    mtmn_config.p_threshold.candidate_number = 20;
    mtmn_config.r_threshold.score = 0.7;
    mtmn_config.r_threshold.nms = 0.7;
    mtmn_config.r_threshold.candidate_number = 10;
    mtmn_config.o_threshold.score = 0.7;
    mtmn_config.o_threshold.nms = 0.7;
    mtmn_config.o_threshold.candidate_number = 2;
    
    face_id_init(&id_list, FACE_ID_SAVE_NUMBER, ENROLL_CONFIRM_TIMES);
    
    Serial.printf("Starting web server on port: '%d'\n", config.server_port);
    if (httpd_start(&camera_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(camera_httpd, &index_uri);
        httpd_register_uri_handler(camera_httpd, &cmd_uri);
        httpd_register_uri_handler(camera_httpd, &led_uri);
        httpd_register_uri_handler(camera_httpd, &detect_uri);
        httpd_register_uri_handler(camera_httpd, &enroll_uri);
        httpd_register_uri_handler(camera_httpd, &recog_uri);
        httpd_register_uri_handler(camera_httpd, &rc_uri);
    }

    config.server_port += 1;
    config.ctrl_port += 1;
    Serial.printf("Starting stream server on port: '%d'\n", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &stream_uri);
    }
}


