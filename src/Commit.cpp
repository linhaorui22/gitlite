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
        sha_blob = Utils::sha1(file, content);
    } 

std::string Blob::get_sha() const {
    return sha_blob;
}

std::string Blob::get_file_name() const {
    return file_name;
}

std::string Blob::get_file_content() const {
    return file_content;
}

std::string Blob::serialize() const {
    std::ostringstream oss;
    oss << sha_blob << "\n" << file_name << "\n";
    oss << file_content;
    return oss.str();
}

Blob Blob::deserialize(const std::string& content){
    std::istringstream iss(content);
    Blob b;
    std::getline(iss, b.sha_blob);
    std::getline(iss, b.file_name);
    std::string rest, all; 
    while (std::getline(iss, rest)) { 
        all += rest; 
        if (!iss.eof()) 
            all += "\n"; 
    } 
    b.file_content = all;
    return b;
}



void Stage_Area::add(const std::string& file_name , const std::string& blob_sha){
    add_staged[file_name] = blob_sha;
    remove_staged.erase(file_name);
}

void Stage_Area::mark_remove(const std::string& file_name){
    remove_staged.insert(file_name);
    add_staged.erase(file_name);
}

void Stage_Area::unmark_remove(const std::string& file_name){
    remove_staged.erase(file_name);
}

void Stage_Area::remove_from_add_staged(const std::string& file_name){
    add_staged.erase(file_name);
}

bool Stage_Area::contains(const std::string& file_name) const {
    return add_staged.find(file_name) != add_staged.end();
}

bool Stage_Area::isRemoved(const std::string& file_name) const {
    return remove_staged.find(file_name) != remove_staged.end();
}

bool Stage_Area::empty() const {
    return add_staged.empty() && remove_staged.empty();
}

void Stage_Area::clear(){
    add_staged.clear();
    remove_staged.clear();
}

const std::map<std::string,std::string>& Stage_Area::files() const {
    return add_staged;
}

const std::set<std::string>& Stage_Area::removedFiles() const {
    return remove_staged;
}

std::string Stage_Area::serialize() const {
    std::ostringstream oss;
    for (auto &file : add_staged) {
        oss <<"A " << file.first << " " << file.second << "\n";
    }
    for (auto &file : remove_staged) { 
        oss << "R " << file << "\n";
    }
    return oss.str();
}

Stage_Area Stage_Area::deserialize(const std::string& raw) {
    Stage_Area s;
    std::istringstream iss(raw);
    std::string line;
    while (std::getline(iss, line)) { 
        if (line.empty()) continue; 
        if (line.rfind("A ",0)==0) { 
            std::istringstream ls(line.substr(2)); 
            std::string fname, bid; 
            ls >> fname >> bid; 
            if (!fname.empty() && !bid.empty()) s.add(fname, bid); 
        } else if (line.rfind("R ",0)==0) { 
            std::string fname = line.substr(2); 
            if (!fname.empty()) s.mark_remove(fname); 
        } 
    }
    return s;
}




Commit::Commit() : timestamp(0) {}

Commit::Commit(const std::string& msg, const std::vector<std::string>& former,
                const std::map<std::string,std::string>& blobs,const Stage_Area& stage,
                const std::time_t& tm):
                message(msg) , timestamp(tm) , formers(former) , blobs_commit(blobs){
                    for(auto file:stage.files()){
                        blobs_commit[file.first] = file.second;
                    }
                    for(auto file:stage.removedFiles()){
                        blobs_commit.erase(file);
                    }
                    std::ostringstream key;
                    key << message << "|" << timestamp;
                    for(auto &f : formers){
                        key << "|" << f;
                    }
                    for(auto &file : blobs_commit){
                        key << "|"<< file.first << ":" << file.second;
                    }
                    id = Utils::sha1(key.str());
                }

Commit Commit::initial_commit(){
    Commit c;
    c.message = "initial commit";
    c.timestamp = 0;
    c.formers = {};
    c.blobs_commit = {};
    c.id = Utils::sha1(c.message + "|" + std::to_string(c.timestamp));
    return c;
}

const std::string& Commit::get_id() const {
    return id;
}

const std::string& Commit::get_message() const {
    return message;
}

const std::time_t& Commit::get_timestamp() const {
    return timestamp;
}
const std::vector<std::string> Commit::get_formers() const {
    return formers;
}

const std::map<std::string,std::string>& Commit::get_blobs_commit() const {
    return blobs_commit;
}

std::string Commit::serialize() const {
    std::ostringstream oss;
    oss << id << "\n";
    oss << message << "\n";
    oss << timestamp << "\n";
    for(auto former : formers){
        oss << "F " << former << "\n";
    }
    for(auto blob : blobs_commit){
        oss << "B " << blob.first << "|" << blob.second << "\n";
    }
    return oss.str();
}

std::string Commit::Time( std::time_t t ){
    std::tm tm_buf;
    localtime_r(&t, &tm_buf);
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y %z", &tm_buf);
    return std::string(buffer);
}


Commit Commit::deserialize(const std::string& raw) {
    Commit c;
    std::istringstream iss(raw);
    std::string line;
    if (!std::getline(iss, c.id)) throw std::runtime_error("Bad commit data");
    if (!std::getline(iss, c.message)) throw std::runtime_error("Bad commit data");
    if (!std::getline(iss, line)) throw std::runtime_error("Bad commit data");
    c.timestamp = static_cast<std::time_t>(std::stoll(line));
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        if (line.rfind("F ", 0) == 0) {
            std::string pid = line.substr(2);
            c.formers.push_back(pid);
        } else if (line.rfind("B ", 0) == 0) {
            std::string rest = line.substr(2);
            size_t pos = rest.find('|');
            if (pos != std::string::npos) { 
                std::string fname = rest.substr(0, pos); 
                std::string bid = rest.substr(pos + 1); 
                if (!fname.empty() && !bid.empty()) { 
                    c.blobs_commit[fname] = bid; 
                } 
            }
        }
    }
    return c;
}
