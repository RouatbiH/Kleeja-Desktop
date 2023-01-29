#include <iostream>
#include <curl/curl.h>
#include <string>
#include <jsoncpp/json/json.h>
#include <regex>

size_t write_data(void* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

std::string decodeHTML(std::string input) {
    std::string output = "";
    std::string buffer = "";
    bool inEntity = false;
    for (int i = 0; i < input.length(); i++) {
        if (input[i] == '&') {
            inEntity = true;
        } else if (input[i] == ';') {
            inEntity = false;
            if (buffer == "lt") {
                output += "<";
            } else if (buffer == "gt") {
                output += ">";
            } else if (buffer == "amp") {
                output += "&";
            } else if (buffer == "quot") {
                output += "\"";
            } else if (buffer == "apos") {
                output += "'";
            } else {
                output += "&" + buffer + ";";
            }
            buffer = "";
        } else if (inEntity) {
            buffer += input[i];
        } else {
            output += input[i];
        }
    }
    return output;
}

void parseJSON(std::string jsonString) {
    Json::Value root;
    Json::Reader reader;
    bool parsingSuccessful = reader.parse(jsonString, root);
    if (!parsingSuccessful) {
        std::cout << "Error parsing JSON" << std::endl;
        exit(1);
    }

    std::regex textareaRegex("<textarea.*?id=\\\"(.*?)\\\".*?>(.*?)</textarea>");
    std::regex labelRegex("<label.*?for=\\\"(.*?)\\\".*?>(.*?)</label>");
    std::smatch match;
    std::smatch labelMatch;
    for (auto& object : root) {
        if (object["t"].asString() == "index_info") {
            std::string i = object["i"].asString();
            while (std::regex_search(i, match, textareaRegex)) {
                if (match.size() >= 3) {
                    std::string id = match[1];
                    std::string text = match[2];
                    if(id == "file1" || id == "delCode" || id == "image1") {
                        std::string labelText;
                        std::string labelSearch = i.substr(0, match.position());
                        if (std::regex_search(labelSearch, labelMatch, labelRegex)) {
                            labelText = labelMatch[2];
                        }
                        if (labelText.empty()) {
							std::cout << id << ": " << decodeHTML(text) << std::endl;
                        } else {
                            std::cout << labelText << ": " << decodeHTML(text) << std::endl;
                        }
                    }
                }
                i = match.suffix().str();
            }
            std::cout << std::endl;
        } else if (object["t"].asString() == "index_err") {
            std::cout << "Error message: " << object["i"].asString() << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {

	const char* URL = "https://up.zalghaym.com/";
	
    // Check if any files were selected
    if (argc < 2) {
        std::cout << "Error: No files selected." << std::endl;
        return 1;
    }

    // Initialize cURL
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cout << "Error: Failed to initialize cURL." << std::endl;
        return 1;
    }

    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, URL);

    // Set POST method
    curl_easy_setopt(curl, CURLOPT_POST, 1);
	
	// Set User-Agent
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Kleeja Desktop/1.0");

    // Create list of files and postfields
    struct curl_httppost* formpost = NULL;
    struct curl_httppost* lastptr = NULL;
    for (int i = 1; i < argc; i++) {
        std::string postfieldName = "file_" + std::to_string(i) + "_";
        curl_formadd(&formpost, &lastptr,
            CURLFORM_COPYNAME, postfieldName.c_str(),
            CURLFORM_FILE, argv[i],
            CURLFORM_END);
    }
        curl_formadd(&formpost, &lastptr,
        CURLFORM_COPYNAME, "ajax",
        CURLFORM_COPYCONTENTS, "1",
        CURLFORM_END);
    curl_formadd(&formpost, &lastptr,
        CURLFORM_COPYNAME, "submitr",
        CURLFORM_COPYCONTENTS, "submit",
        CURLFORM_END);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    // Perform the request
    CURLcode res = curl_easy_perform(curl);

    // Cleanup
    curl_easy_cleanup(curl);
    curl_formfree(formpost);

    // Check for errors
    if (res != CURLE_OK) {
        std::cout << "Error: " << curl_easy_strerror(res) << std::endl;
        return 1;
    }
    
	parseJSON(response);
	
	std::cout << "File(s) uploaded successfully." << std::endl;
    return 0;
}

//g++ -std=c++11 test5.cpp -o test5 -ljsoncpp -lcurl
