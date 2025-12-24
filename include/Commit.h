#ifndef COMMIT_H
#define COMMIT_H

#include<iostream>
#include<map>
#include<vector>
#include<string>
#include<cmath>
#include<ctime>
#include<set>

class Blob{
private :
    std::string sha_blob;
    std::string file_name;
    std::string file_content;
public :
    Blob() = default;
    Blob(const std::string& file,const std::string content);
    std::string get_sha() const;
    std::string get_file_name() const;
    std::string get_file_content() const;
    std::string serialize() const;
    static Blob deserialize(const std::string& content);
};

class Stage_Area{
private:
    std::map<std::string, std::string> add_staged; // filename -> blobId
    std::set<std::string> remove_staged;
public:
    void add(const std::string& file_name, const std::string& blob_sha);
    void mark_remove(const std::string& file_name);
    void unmark_remove(const std::string& file_name);
    void remove_from_add_staged(const std::string& file_name);

    bool contains(const std::string& file_name) const;
    bool isRemoved(const std::string& file_name) const;
    bool empty() const;
    void clear();

    const std::map<std::string, std::string>& files() const;
    const std::set<std::string>& removedFiles() const;

    std::string serialize() const;
    static Stage_Area deserialize(const std::string& raw);
};

class Commit{
private :
    std::string id;
    std::string message;
    std::time_t timestamp;
    std::vector<std::string> formers;
    std::map<std::string,std::string> blobs_commit;

public:
    Commit();
    Commit(const std::string& msg,const std::vector<std::string>& former ,
            const std::map<std::string,std::string>& blobs,const Stage_Area& stage,
            const std::time_t& tm = std::time(nullptr));
    static Commit initial_commit();
    const std::string& get_id() const;
    const std::string& get_message() const;
    const std::time_t& get_timestamp() const;
    const std::vector<std::string> get_formers() const;
    const std::map<std::string,std::string>& get_blobs_commit() const;

    std::string serialize() const;
    static Commit deserialize(const std::string& raw);
    static std::string Time(std::time_t t);
    
    
};



#endif // COMMIT_H
