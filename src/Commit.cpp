#include "../include/Commit.h"
#include "../include/Utils.h"
#include <sstream>
#include <algorithm>
#include<iostream>
#include<map>
#include<vector>
#include<string>
#include<cmath>
#include<ctime>

Blob::Blob(const std::string& file,const std::string content):
    file_name(file) , file_content(content) {
        std::vector<unsigned char> data = Utils::readContents(content);
        sha_blob(Utils::sha1(data));
    } 

std::string Blob::get_sha(){
    return sha_blob;
}

std::string Blob::get_file_name(){
    return file_name;
}

std::string Blob::get_file_content(){
    return file_content;
}

std::string Blob::serialize() const {
    std::ostringstream oss;
    oss << sha_blob << "\n" << file_name << "\n";
    oss << file_content;
    return oss.str();
}

