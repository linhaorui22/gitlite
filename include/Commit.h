#ifndef COMMIT_H
#define COMMIT_H

#include<iostream>
#include<map>
#include<vector>
#include<string>
#include<cmath>
#include<ctime>

class Blob{
private :
    std::string sha_blob;
    std::string file_name;
    std::string file_content;
public :
    Blob() = default;
    Blob(const std::string& file,const std::string content);
    std::string get_sha();
    std::string get_file_name();
    std::string get_file_content();
    std::string serialize() const;
    //static Blob deserialize(const std::string& content);
};

class Stage_Area{
private:
    std::map<std::string, std::string> staged; // filename -> blobId
public:
    void add(const std::string& file_name, const std::string& blob_sha);
    void remove(const std::string& file_name);
    bool contains(const std::string& file_name) const;
    bool empty() const;
    void clear();

    const std::map<std::string, std::string>& files() const;

    std::string serialize() const;
    //static Stage deserialize(const std::string& raw);
};

class Commit{

};



#endif // COMMIT_H
