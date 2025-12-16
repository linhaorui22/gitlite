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

void Stage_Area::add(const std::string& file_name , const std::string& blob_sha){
    staged[file_name] = blob_sha;
}

void Stage_Area::remove(const std::string& file_name){
    staged.erase(file_name);
}

bool Stage_Area::contains(const std::string& file_name) const {
    return staged.find(file_name)!=staged.end();
}

bool Stage_Area::empty() const {
    if(staged.empty()){
        return true;
    }
    return false;
}

void Stage_Area::clear(){
    staged.clear();
}

const std::map<std::string,std::string>& Stage_Area::files() const {
    return staged;
}

std::string Stage_Area::serialize() const {
    std::ostringstream oss;
    for (auto &flie : staged) {
        oss << file.first << " " << file.second << "\n";
    }
    return oss.str();
}
