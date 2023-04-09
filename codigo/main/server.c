#include "server.h"


// Definitions ------------------------------------------------------------------------------------

// Only use this for same type values
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define SERVER_TAG ("WEB_SERVER")
// For reading and writing files
#define BUFFER_LEN (2048)


// Global variables definition --------------------------------------------------------------------

static const char* html_type = "text/html";
static const char* js_type = "application/javascript";
static const char* svg_type = "image/svg+xml";
static const char* css_type = "text/css";
static const char* json_type = "application/json";


// Static functions declaration -------------------------------------------------------------------
// Static helper functions ----------------------------------------------------

static esp_err_t send_chunked(httpd_req_t* req, const char* filename);
static int get_param(const char* uri, const char* key, char* value);
static bool file_exists(char* filename);
static const char* get_type(char* filename);
static int parse_json_data(const char* json, const char* key, char* out_value);


// Handler functions ----------------------------------------------------------

static esp_err_t index_handler(httpd_req_t* req);
static esp_err_t upload_handler(httpd_req_t* req);
static esp_err_t static_handler(httpd_req_t* req);

static esp_err_t api_get_ap_info_handler(httpd_req_t* req);
static esp_err_t api_get_sta_info_handler(httpd_req_t* req);
static esp_err_t api_post_sta_info_handler(httpd_req_t* req);
static esp_err_t api_get_controller_info_handler(httpd_req_t* req);
static esp_err_t api_send_power_handler(httpd_req_t* req);
static esp_err_t api_send_set_point_handler(httpd_req_t* req);


// Static functions definition --------------------------------------------------------------------
// Static helper functions ----------------------------------------------------

static esp_err_t send_chunked(httpd_req_t* req, const char* filename)
{
    FILE* fd = fopen(filename, "r");
    if (fd == NULL) {
        ESP_LOGE(SERVER_TAG, "Failed to open %s", filename);
        return ESP_FAIL;
    }

    char buffer[BUFFER_LEN + 1] = {0};

    int bytes_read = BUFFER_LEN;
    while (bytes_read == BUFFER_LEN) {
        bytes_read = fread(buffer, 1, BUFFER_LEN, fd);
        int ret = httpd_resp_send_chunk(req, buffer, bytes_read);

        for (int i=0; i<10 && ret != ESP_OK; i++) {
            ret = httpd_resp_send_chunk(req, buffer, bytes_read);
            vTaskDelay(4);
        }

        if (ret != ESP_OK) {
            ESP_LOGE(SERVER_TAG, "Failed to send %s (%s)",
                filename, esp_err_to_name(ret));
            return ret;
        }
    }

    fclose(fd);
    return httpd_resp_send_chunk(req, NULL, 0);
}

static int get_param(const char* uri, const char* key, char* value)
{
    int started = 0;
    for (int i=0; i<strlen(uri) - strlen(key); i++) {
        const char* text = uri + i;
        if (text[0] == '?') {
            started = 1;
        }

        if (started) {
            if (strncmp(text, key, strlen(key)) == 0) {
                for (int j=strlen(key)+1; j<strlen(text); j++) {
                    int k = j - (strlen(key)+1);
                    if (text[j] == '&' || text[j] == '\0') {
                        value[k] = '\0';
                        break;
                    }
                    value[k] = text[j];
                }
                return 1;
            }
        }
    }

    return 0;
}

static bool file_exists(char* filename)
{
    struct stat buffer;
    return stat(filename, &buffer) == 0;
}

static const char* get_type(char* filename)
{
    int len = strlen(filename);
    
    for (int i=len-1; i>len-8 && i>0; i--) {
        if (filename[i] == '.') {
            char* extension = &(filename[i]);

            if (strncmp(extension, ".html", 5) == 0) {
                return html_type;
            }

            if (strncmp(extension, ".js", 3) == 0) {
                return js_type;
            }

            if (strncmp(extension, ".svg", 4) == 0) {
                return svg_type;
            }

            if (strncmp(extension, ".css", 4) == 0) {
                return css_type;
            }

            break;
        }
    }

    ESP_LOGE(SERVER_TAG, "Could not find the extension for %s", filename);
    return html_type;
}

static int parse_json_data(const char* json, const char* key, char* out_value)
{
    int json_len = strlen(json);
    int key_len = strlen(key);
    int starting_point = -1;

    for (int i=0; i < json_len - key_len; i++) {
        if (strncmp(&json[i], key, key_len) == 0) {
            starting_point = i + key_len + 2;
            break;
        }
    }

    if (starting_point == -1) {
        return -1;
    }

    for (int i=starting_point, j=0; i<json_len; i++, j++) {
        if (json[i] == ',' || json[i] == '}') {
            out_value[j] = '\0';
            return j;
        }
        out_value[j] = json[i];
    }

    return -1;
}

// Handler functions ----------------------------------------------------------

static esp_err_t index_handler(httpd_req_t* req)
{
    ESP_LOGI(SERVER_TAG, "Serving index (%s)", html_type);

    httpd_resp_set_type(req, html_type);
    return send_chunked(req, "/spiffs/index.html");
}

static esp_err_t upload_handler(httpd_req_t* req)
{
    char recv_filename[128] = {0};
    if (!get_param(req->uri, "filename", recv_filename)) {
        char* resp = "Please provide a file name with the uri (?filename=x)";
        return httpd_resp_send(req, resp, strlen(resp));
    }

    char filename[140] = {0};
    sprintf(filename, "/spiffs/%s", recv_filename);
    ESP_LOGI(SERVER_TAG, "Receiving %s", recv_filename);
    
    FILE* fd = fopen(filename, "w");
    if (fd == NULL) {
        ESP_LOGE(SERVER_TAG, "Failed to open file %s for writing", filename);
        return ESP_FAIL;
    }

    char buffer[256 + 1] = {0};
    unsigned int content_unread = req->content_len;
    while (content_unread > 0) {
        int recv_size = MIN(content_unread, 256);
        int ret = httpd_req_recv(req, buffer, recv_size);
        if (ret <= 0) {
            ESP_LOGE(SERVER_TAG, "Connection closed, transfer failed");
            fclose(fd);
            return ESP_FAIL;
        }

        fwrite(buffer, 1, recv_size, fd);
        content_unread -= recv_size;
    }

    fclose(fd);
    ESP_LOGI(SERVER_TAG, "%s transfer finished", filename);
    const char resp[] = "Transfer finished :)";
    return httpd_resp_send(req, resp, strlen(resp));
}

static esp_err_t static_handler(httpd_req_t* req)
{
    char filename[HTTPD_MAX_URI_LEN + 15] = {0};

    sprintf(filename, "/spiffs%s", req->uri);
    const char* content_type = get_type(filename);

    ESP_LOGI(SERVER_TAG, "serving %s, (%s)", filename, content_type);

    if (file_exists(filename)) {
        httpd_resp_set_type(req, content_type);
        return send_chunked(req, filename);
    }

    sprintf(filename, "/spiffs%s.gz", req->uri);
    if (file_exists(filename)) {
        httpd_resp_set_type(req, content_type);
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
        return send_chunked(req, filename);
    }

    char* resp = "File not found :(";
    httpd_resp_set_status(req, "404 Not Found");
    return httpd_resp_send(req, resp, strlen(resp));
}


// API handlers ---------------------------------------------------------------

static esp_err_t api_get_ap_info_handler(httpd_req_t* req)
{
    ESP_LOGI(SERVER_TAG, "Serving api get ap info (%s)", json_type);

    NetInfo info = ap_info();
    char resp[2*NET_CRED_MAX_LEN + 20 + 50] = {0};
    sprintf(resp, "{\"ssid\":\"%s\",\"password\":\"%s\",\"ip\":\"%s\"}",
        info.ssid, info.password, info.ip);

    httpd_resp_set_type(req, json_type);
    return httpd_resp_send(req, resp, strlen(resp));
}

static esp_err_t api_get_sta_info_handler(httpd_req_t* req)
{
    ESP_LOGI(SERVER_TAG, "Serving api get sta info (%s)", json_type);

    NetInfo info = sta_info();
    char resp[2*NET_CRED_MAX_LEN + 20 + 50] = {0};
    sprintf(resp, "{\"ssid\":\"%s\",\"password\":\"%s\",\"ip\":\"%s\"}",
        info.ssid, info.password, info.ip);

    httpd_resp_set_type(req, json_type);
    return httpd_resp_send(req, resp, strlen(resp));
}

static esp_err_t api_post_sta_info_handler(httpd_req_t* req)
{
    ESP_LOGI(SERVER_TAG, "Wifi sta info posted");

    char data[256] = {0};
    httpd_req_recv(req, data, req->content_len);

    char ssid[NET_CRED_MAX_LEN + 2] = {0};
    char password[NET_CRED_MAX_LEN + 2] = {0};

    if (parse_json_data(data, "ssid", ssid) == -1) {
        ESP_LOGE(SERVER_TAG, "Could not find json field \"ssid\"");

        httpd_resp_set_status(req, "400 Bad request");
        char* resp = "Could not find the ssid field in json";
        return httpd_resp_send(req, resp, strlen(resp));
    }

    if (parse_json_data(data, "password", password) == -1) {
        ESP_LOGE(SERVER_TAG, "Could not find json field \"password\"");

        httpd_resp_set_status(req, "400 Bad request");
        char* resp = "Could not find the password field in json";
        return httpd_resp_send(req, resp, strlen(resp));
    }

    sprintf(ssid, "%.*s", strlen(ssid)-2, &ssid[1]);        // Removing the quotes
    sprintf(password, "%.*s", strlen(password)-2, &password[1]);

    char* resp = "Changing Wifi info";
    esp_err_t err = httpd_resp_send(req, resp, strlen(resp)); 

    update_config(ssid, password);
    return err;
}

static esp_err_t api_get_controller_info_handler(httpd_req_t* req)
{
    ESP_LOGI(SERVER_TAG, "Serving api get controller info (%s)", json_type);

    char readings[64];
    sprintf(readings,
            "[%d,%d,%d,%d,%d,%d,%d,%d]",
            get_reading(0),
            get_reading(1),
            get_reading(2),
            get_reading(3),
            get_reading(4),
            get_reading(5),
            get_reading(6),
            get_reading(7));

    ControllerInfo info = controller_info();
    char resp[256];
    sprintf(resp,
            "{\"readings\":%s,\"steady_state\":%s,\"power_ratio\":[%d,%d],\"control\":%s,\"set_point\":%d}",
            readings,
            info.steady? "true" : "false",
            info.power_ratio[0],
            info.power_ratio[1],
            info.control? "true" : "false",
            info.target);

    httpd_resp_set_type(req, json_type);
    return httpd_resp_send(req, resp, strlen(resp));
}

static esp_err_t api_send_power_handler(httpd_req_t* req)
{
    ESP_LOGI(SERVER_TAG, "Update power posted");

    char data[256] = {0};
    httpd_req_recv(req, data, req->content_len);

    char power[16] = {0};

    if (parse_json_data(data, "power", power) == -1) {
        ESP_LOGE(SERVER_TAG, "Could not find power field in json");

        httpd_resp_set_status(req, "400 Bad request");
        char* resp = "Could not find the power field in json";
        return httpd_resp_send(req, resp, strlen(resp));
    }

    int cycles_on = atoi(power);
    set_ratio(cycles_on);

    char* resp = "Entering uncontrolled mode and updating power";
    return httpd_resp_send(req, resp, strlen(resp)); 
}

static esp_err_t api_send_set_point_handler(httpd_req_t* req)
{
    ESP_LOGI(SERVER_TAG, "Update target posted");

    char data[256] = {0};
    httpd_req_recv(req, data, req->content_len);

    char target_str[16] = {0};

    if (parse_json_data(data, "set_point", target_str) == -1) {
        ESP_LOGE(SERVER_TAG, "Could not find set_point field in json");

        httpd_resp_set_status(req, "400 Bad request");
        char* resp = "Could not find the set_point field in json";
        return httpd_resp_send(req, resp, strlen(resp));
    }

    int target = atoi(target_str);
    set_target(target);

    char* resp = "Entering controlled mode and updating target temp";
    return httpd_resp_send(req, resp, strlen(resp)); 
}


// URIs creation ----------------------------------------------------------------------------------

httpd_uri_t uri_upload = {
    .uri = "/upload_file",
    .method = HTTP_POST,
    .handler = upload_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_index = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = index_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_index_alt = {
    .uri = "/index.html",
    .method = HTTP_GET,
    .handler = static_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_connection_html = {
    .uri = "/connection.html",
    .method = HTTP_GET,
    .handler = static_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_connection_js = {
    .uri = "/connection.js",
    .method = HTTP_GET,
    .handler = static_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_c3_css = {
    .uri = "/c3.min.css",
    .method = HTTP_GET,
    .handler = static_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_c3_js = {
    .uri = "/c3.min.js",
    .method = HTTP_GET,
    .handler = static_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_control_html = {
    .uri = "/control.html",
    .method = HTTP_GET,
    .handler = static_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_control_js = {
    .uri = "/control.js",
    .method = HTTP_GET,
    .handler = static_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_d3_js = {
    .uri = "/d3.v4.min.js",
    .method = HTTP_GET,
    .handler = static_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_fire_icon = {
    .uri = "/fire_icon.svg",
    .method = HTTP_GET,
    .handler = static_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_style_css = {
    .uri = "/style.css",
    .method = HTTP_GET,
    .handler = static_handler,
    .user_ctx = NULL
};

// API uris -------------------------------------------------------------------

httpd_uri_t uri_get_ap_info = {
    .uri = "/api/ap_info",
    .method = HTTP_GET,
    .handler = api_get_ap_info_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_get_sta_info = {
    .uri = "/api/sta_info",
    .method = HTTP_GET,
    .handler = api_get_sta_info_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_post_sta_info = {
    .uri = "/api/sta_info",
    .method = HTTP_POST,
    .handler = api_post_sta_info_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_get_controller_info = {
    .uri = "/api/controller_info",
    .method = HTTP_GET,
    .handler = api_get_controller_info_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_send_power = {
    .uri = "/api/send_power",
    .method = HTTP_POST,
    .handler = api_send_power_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_send_set_point = {
    .uri = "/api/send_set_point",
    .method = HTTP_POST,
    .handler = api_send_set_point_handler,
    .user_ctx = NULL
};


// Public functions definition --------------------------------------------------------------------

httpd_handle_t server_init(int task_prio)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.task_priority = task_prio;
    config.max_uri_handlers = 20;
    config.stack_size = 16384;

    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_upload);
        httpd_register_uri_handler(server, &uri_index);
        httpd_register_uri_handler(server, &uri_index_alt);
        httpd_register_uri_handler(server, &uri_connection_html);
        httpd_register_uri_handler(server, &uri_connection_js);
        httpd_register_uri_handler(server, &uri_c3_css);
        httpd_register_uri_handler(server, &uri_c3_js);
        httpd_register_uri_handler(server, &uri_control_html);
        httpd_register_uri_handler(server, &uri_control_js);
        httpd_register_uri_handler(server, &uri_d3_js);
        httpd_register_uri_handler(server, &uri_fire_icon);
        httpd_register_uri_handler(server, &uri_style_css);
        httpd_register_uri_handler(server, &uri_get_ap_info);
        httpd_register_uri_handler(server, &uri_get_sta_info);
        httpd_register_uri_handler(server, &uri_post_sta_info);
        httpd_register_uri_handler(server, &uri_get_controller_info);
        httpd_register_uri_handler(server, &uri_send_power);
        httpd_register_uri_handler(server, &uri_send_set_point);
    }

    return server;
}
