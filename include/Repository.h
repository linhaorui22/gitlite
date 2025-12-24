#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <string>
#include <vector>
#include <map>
#include "Commit.h"

class Repository{

private:
    std::string repoDir;
    std::string objectDir;
    std::string refsDir;
    std::string headPath;
    std::string indexPath;

    std::string branch_now() const ;
    void ensure() const;
    void write_ref(const std::string& branch , const std::string& commit_id) const;
    std::string read_ref(const std::string& branch) const;
    void save_object(const std::string& id , const std::string& data) const;
    std::string load_object(const std::string& id) const;
    bool object_exist(const std::string& id) const;
    void save_blob(const Blob& b) const;
    Blob load_blob(const std::string& blob_id) const;
    Stage_Area read_stage() const;
    void write_stage(const Stage_Area& s) const;
    void clear_stage() const;
    Commit load_commit_by_id(const std::string& idcommit) const;
    std::vector<std::string> all_commit_ids() const;

public:
    Repository(const std::string& dir = ".gitlite");
    static std::string getGitliteDir();
    void init();
    void add(const std::string& file_name);
    void commit(const std::string& message);
    void rm(const std::string& file_name);
    void log() const;
    void globalLog() const;
    void find(const std::string& message) const;
    void checkoutFile(const std::string& commit_id, const std::string& file_name);
    void checkoutBranch(const std::string& branch_name);
    void checkoutFile(const std::string& file_name); 
    void checkoutFileInCommit(const std::string& commit_id, 
        const std::string& file_name);
    void status() const;
    void branch(const std::string& name);
    void rm_branch(const std::string& name);
    void reset(const std::string& commit_id);
    void merge(const std::string& branch_name);
    

};









#endif // REPOSITORY_H
