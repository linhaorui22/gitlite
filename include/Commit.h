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
    std::map<std::string,std::string> add_stage;//name->sha
    std::map<std::string , std::string> remove_stage;//name->sha

public:
    void add(const std::string& file_name);
    void remove(const std::string& file_name);
};

class Commit{

};



#endif // COMMIT_H
