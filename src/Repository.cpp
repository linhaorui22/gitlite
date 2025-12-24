#include "../include/Repository.h"
#include "../include/Utils.h"
#include "../include/GitliteException.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

Repository::Repository(const std::string& dir)
    : repoDir(dir),
      objectDir(Utils::join(dir, "objects")),
      refsDir(Utils::join(dir, "refs")),
      headPath(Utils::join(dir, "HEAD")),
      indexPath(Utils::join(dir, "index")) {}

std::string Repository::branch_now() const {
    if(!Utils::exists(headPath)){
        return "master";
    }
    else {
        return Utils::readContentsAsString(headPath);
    }
}

void Repository::ensure() const {
    if(!Utils::isDirectory(repoDir)){
        Utils::exitWithMessage("Not in an initialized Gitlite directory.");
    }
}

void Repository::write_ref(const std::string& branch , const std::string& commit_id) const {
    Utils::writeContents(Utils::join(refsDir,branch),commit_id);
}

std::string Repository::read_ref(const std::string& branch) const {
    std::string path = Utils::join(refsDir, branch);
    if (!Utils::exists(path)) {
        return "";
    }
    return Utils::readContentsAsString(path);
}

void Repository::save_object(const std::string& id , const std::string& data) const {
    Utils::writeContents(Utils::join(objectDir,id),data);
}

std::string Repository::load_object(const std::string& id) const {
    std::string path = Utils::join(objectDir,id);
    if(!Utils::exists(path)){
        return "";
    }
    return Utils::readContentsAsString(path);
}

bool Repository::object_exist(const std::string& id) const {
    return Utils::exists(Utils::join(objectDir,id));
}

void Repository::save_blob(const Blob& b) const {
    if(!object_exist(b.get_sha())){
        save_object(b.get_sha(),b.serialize());
    }
}

Blob Repository::load_blob(const std::string& blob_id) const {
    std::string content = load_object(blob_id);
    return Blob::deserialize(content); 
}

Stage_Area Repository::read_stage() const {
    if(Utils::exists(indexPath) == false){
        return Stage_Area();
    }
    std::string s = Utils::readContentsAsString(indexPath);
    return Stage_Area::deserialize(s);
}

void Repository::write_stage(const Stage_Area& s) const {
    Utils::writeContents(indexPath,s.serialize());
}

void Repository::clear_stage() const {
    if(Utils::exists(indexPath)){
        Utils::restrictedDelete(indexPath);
    }
}

Commit Repository::load_commit_by_id(const std::string& idcommit) const {
    if(idcommit.size() == Utils::UID_LENGTH){
        std::string raw = load_object(idcommit);
        return Commit::deserialize(raw);
    }
    auto file = Utils::plainFilenamesIn(objectDir);
    for(auto &f : file){
        if(f.rfind(idcommit,0) == 0){
            std::string raw = load_object(f);
            return Commit::deserialize(raw);
        }
    }
    Utils::exitWithMessage("No commit with that id exists.");
    return Commit();
}

std::vector<std::string> Repository::all_commit_ids() const {
    return Utils::plainFilenamesIn(objectDir);
}

std::string Repository::getGitliteDir() {
    return ".gitlite";
}













void Repository::init() {
    if (Utils::isDirectory(repoDir)) {
        Utils::exitWithMessage("A Gitlite version-control system already exists in the current directory.");
    }
    Utils::createDirectories(repoDir);
    Utils::createDirectories(objectDir);
    Utils::createDirectories(refsDir);
    Utils::writeContents(headPath, "master");

    Commit initial = Commit::initial_commit();
    save_object(initial.get_id(), initial.serialize());
    write_ref("master", initial.get_id());
}

void Repository::add(const std::string& file_name){
    ensure();
    if(!Utils::isFile(file_name)){
        Utils::exitWithMessage("File does not exist.");
    }
    std::string content = Utils::readContentsAsString(file_name); 
    Blob b(file_name,content);
    save_blob(b);
    std::string sha_blob = b.get_sha();

    std::string branch = branch_now();
    std::string head_id = read_ref(branch);
    std::map<std::string,std::string> tracked;
    if(!head_id.empty()){
        Commit head = Commit::deserialize(load_object(head_id));
        tracked = head.get_blobs_commit();
    }

    Stage_Area s = read_stage();

    if(s.isRemoved(file_name)){
        s.unmark_remove(file_name);
        s.add(file_name,sha_blob);
        write_stage(s);
        return;
    }

    if(tracked.find(file_name) != tracked.end() && tracked[file_name] == sha_blob){
        s.remove_from_add_staged(file_name);
        write_stage(s);
        return;
    }

    s.add(file_name,sha_blob);
    write_stage(s);
}




















void Repository::commit(const std::string& message) {
    ensure();
    Stage_Area s = read_stage();
    if(s.empty()){
        Utils::exitWithMessage("No changes added to the commit.");
    }
    if(message.empty()){
        Utils::exitWithMessage("Please enter a commit message.");
    }
    std::string branch = branch_now();
    std::string former = read_ref(branch);
    std::map<std::string,std::string> blob_commit;
    if (!former.empty()) {
        std::string raw = load_object(former);
        Commit parent = Commit::deserialize(raw);
        blob_commit = parent.get_blobs_commit();
    }
    Commit new_commit(message,former.empty() ? std::vector<std::string>() : std::vector<std::string> {former} ,
                        blob_commit,s);
    save_object(new_commit.get_id() , new_commit.serialize() );
    write_ref(branch,new_commit.get_id());
    clear_stage();
}

void Repository::rm(const std::string& file_name){
    ensure();
    Stage_Area s = read_stage();
    std::string branch = branch_now();
    std::string head_id = read_ref(branch);
    std::map<std::string,std::string> t;
    if(head_id.empty() == false){
        Commit head = Commit::deserialize(load_object(head_id));
        t = head.get_blobs_commit();
    }
    bool flag_stage = s.contains(file_name);
    bool flag_t = (t.find(file_name) != t.end());
    if(flag_stage == false && flag_t == false){
        Utils::exitWithMessage("No reason to remove the file.");
    }
    if(flag_stage == true && flag_t == false){
        s.remove_from_add_staged(file_name);
        write_stage(s);
        return;
    }
    if(flag_t == true){
        s.mark_remove(file_name);
        write_stage(s);
        if(Utils::isFile(file_name)){
            Utils::restrictedDelete(file_name);
        }
    }
}

void Repository::log() const {
    ensure();
    std::string branch = Utils::readContentsAsString(headPath);
    std::string commit_id = read_ref(branch);
    while(commit_id.empty() == false){
        std::string raw = load_object(commit_id);
        Commit c = Commit::deserialize(raw);
        std::cout<<"===\n";
        std::cout<<"commit "<<c.get_id()<<"\n";
        if(c.get_formers().size() >= 2){
            std::cout<<"Merge:"<<c.get_formers()[0].substr(0,7)<<" "<<c.get_formers()[1].substr(0,7)<<"\n";
        }
        std::cout<<"Date:"<<Commit::Time(c.get_timestamp())<<"\n";
        std::cout<<c.get_message()<<"\n\n";
        if(c.get_formers().empty()){
            break;
        }
        commit_id = c.get_formers()[0];
    }
}

void Repository::globalLog() const {
    ensure();
    auto files = all_commit_ids();
    for(auto &f : files ){
        std::string raw = load_object(f);
        Commit c = Commit::deserialize(raw);
        std::cout<<"===\n";
        std::cout<<"commit "<<c.get_id()<<"\n";
        if(c.get_formers().size() >= 2){
            std::cout<<"Merge:"<<c.get_formers()[0].substr(0,7)<<" "<<c.get_formers()[1].substr(0,7)<<"\n";
        }
        std::cout<<"Date:"<<Commit::Time(c.get_timestamp())<<"\n";
        std::cout<<c.get_message()<<"\n\n";
    }
}

void Repository::find(const std::string& message) const {
    ensure();
    auto files = all_commit_ids();
    bool flag = false;
    for(auto &f : files){
        Commit c = Commit::deserialize(load_object(f));
        if(c.get_message() == message){
            flag = true;
            std::cout<<c.get_id()<<"\n";
        }
    }
    if(flag == false){
        Utils::exitWithMessage("Found no commit with that message.");
    }
}

void Repository::checkoutFile(const std::string& commit_id , const std::string& file_name){
    ensure();
    Commit c = load_commit_by_id(commit_id);
    auto b = c.get_blobs_commit();
    if(b.find(file_name) == b.end()){
        Utils::exitWithMessage("File does not exist in that commit.");
    }
    std::string blob_id = b.at(file_name);
    std::string content = load_object(blob_id);
    Utils::writeContents(file_name,content);
}

void Repository::checkoutBranch(const std::string& branch_name){
    ensure();
    std::string path = Utils::join(refsDir,branch_name);
    if(Utils::exists(path) == false){
        Utils::exitWithMessage("No such branch exists.");
    }
    std::string now = branch_now();
    if(now == branch_name){
        Utils::exitWithMessage("No need to checkout the current branch.");
    }
    std::string target_id = read_ref(branch_name);
    if(target_id.empty()){
        Utils::exitWithMessage("No commit with that id exists.");
    }
    Commit target = Commit::deserialize(load_object(target_id));
    std::string now_commit_id = read_ref(now);
    std::map<std::string,std::string> blob_now;
    if(now_commit_id.empty() == false){
        Commit now_commit = Commit::deserialize(load_object(now_commit_id));
        blob_now = now_commit.get_blobs_commit();
    }
    for(auto &f : target.get_blobs_commit()){
        const std::string& f_name = f.first;
        if(Utils::exists(f_name) == false){
            continue;
        } 
        if(blob_now.find(f_name) == blob_now.end()){
            Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
        }
    }
    for(auto &f : target.get_blobs_commit()){
        std::string f_name = f.first;
        std::string blob_id = f.second;
        std::string content = load_object(blob_id);
        Utils::writeContents(f_name, content);
    }
    for(auto &f : blob_now){
        if(target.get_blobs_commit().find(f.first) == target.get_blobs_commit().end()){
            if(Utils::isFile(f.first)){
                Utils::restrictedDelete(f.first);
            }
        }
    }
    Utils::writeContents(headPath,branch_name);
    clear_stage();
}

void Repository::checkoutFile(const std::string& file_name){
    ensure();
    std::string now_branch = branch_now();
    std::string commit_id = read_ref(now_branch);
    if(commit_id.empty()){
        Utils::exitWithMessage("No commit with that id exists.");
    }
    Commit c = Commit::deserialize(load_object(commit_id));
    auto blob = c.get_blobs_commit();
    if(blob.find(file_name) == blob.end()){
        Utils::exitWithMessage("File does not exist in that commit.");
    }
    std::string blob_id = blob.at(file_name);
    std::string content = load_object(blob_id);
    Utils::writeContents(file_name,content);
}

void Repository::checkoutFileInCommit(const std::string& commit_id , const std::string& file_name){
    ensure();
    Commit c = load_commit_by_id(commit_id);
    auto blob = c.get_blobs_commit();
    if(blob.find(file_name) == blob.end()){
        Utils::exitWithMessage("File does not exist in that commit.");
    }
    std::string blob_id = blob.at(file_name);
    std::string content = load_object(blob_id);
    Utils::writeContents(file_name,content);
}

void Repository::status() const {
    ensure();
    //branch
    std::cout<<"=== Branches ===\n";
    auto branches = Utils::plainFilenamesIn(refsDir);
    std::string now = Utils::readContentsAsString(headPath);
    std::sort(branches.begin(),branches.end());
    for(auto &b : branches){
        if(b == now){
            std::cout<<"*"<<b<<"\n";
        }
        else {
            std::cout<<b<<"\n";
        }
    }
    std::cout<<"\n";
    //stage
    std::cout<<"=== Staged Files ===\n";
    Stage_Area s = read_stage();
    for(auto &f : s.files()){
        std::cout<<f.first<<"\n";
    } 
    std::cout<<"\n";
    //remove
    std::cout<<"=== Removed Files ===\n";
    for(auto &f : s.removedFiles()){
        std::cout<<f<<"\n";
    }
    std::cout<<"\n";

}



void Repository::branch(const std::string& name){
    ensure();
    std::string path = Utils::join(refsDir,name);
    if(Utils::exists(path)){
        Utils::exitWithMessage("A branch with that name already exists.");
    }
    std::string head_commit = read_ref(branch_now());
    write_ref(name,head_commit);
}


void Repository::rm_branch(const std::string& name){
    ensure();
    std::string path = Utils::join(refsDir,name);
    if(!Utils::exists(path)){
        Utils::exitWithMessage("A branch with that name does not exist.");
    }
    std::string now = branch_now();
    if(now == name){
        Utils::exitWithMessage("Cannot remove the current branch.");
    }
    Utils::restrictedDelete(path);
}

void Repository::reset(const std::string& commit_id){
    ensure();
    Commit target = load_commit_by_id(commit_id);
    std::string now_branch = branch_now();
    std::string now_commit_id = read_ref(now_branch);
    std::map<std::string,std::string> now_blob;
    if(!now_commit_id.empty()){
        Commit now_commit = Commit::deserialize(load_object(now_commit_id));
        now_blob = now_commit.get_blobs_commit();
    }
    for(auto &f: target.get_blobs_commit()){
        const std::string& f_name = f.first;
        if(!Utils::exists(f_name)){
            continue;
        }
        if(now_blob.find(f_name) == now_blob.end()){
            Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
        }
    }
    for(auto &f : target.get_blobs_commit()){
        std::string content = load_object(f.second);
        Utils::writeContents(f.first,content);
    }
    for(auto &f : now_blob){
        if(target.get_blobs_commit().find(f.first) == target.get_blobs_commit().end()){
            if(Utils::isFile(f.first)){
                Utils::restrictedDelete(f.first);
            }
        }
    }
    write_ref(now_branch,target.get_id());
    clear_stage();
}

void Repository::merge(const std::string& branch_name){
    ensure();
    Stage_Area s = read_stage();
    if(!s.empty()){
        Utils::exitWithMessage("You have uncommitted changes.");
    }
    std::string now = branch_now();
    std::string given_ref = read_ref(branch_name);
    if(given_ref.empty()){
        Utils::exitWithMessage("A branch with that name does not exist.");
    }
    if(now == branch_name){
        Utils::exitWithMessage("Cannot merge a branch with itself.");
    }
    std::string head_id = read_ref(now);
    Commit head = Commit::deserialize(load_object(head_id));
    Commit given = Commit::deserialize(load_object(given_ref));
    std::set<std::string> head_former;
    std::vector<std::string> stack = {head.get_id()};
    while(!stack.empty()){
        std::string id = stack.back();
        stack.pop_back();
        if(head_former.count(id)){
            continue;
        }
        head_former.insert(id);
        Commit c = Commit::deserialize(load_object(id));
        for(auto &p : c.get_formers()){
            stack.push_back(p);
        }
    }
    std::string split_id;
    std::vector<std::string> q = {given.get_id()};
    std::set<std::string> v;
    while(!q.empty()){
        std::string id = q.front();
        q.erase(q.begin());
        if(head_former.count(id)){
            split_id = id;
            break;
        }
        if(v.count(id)){
            continue;
        }
        v.insert(id);
        Commit c = Commit::deserialize(load_object(id));
        for(auto &p : c.get_formers()){
            q.push_back(p);
        }
    }
    if(split_id == given.get_id()){
        Utils::exitWithMessage("Given branch is an ancestor of the current branch.");
        return;
    }
    if(split_id == head_id){
        write_ref(now,given.get_id());
        Utils::exitWithMessage("Current branch fast-forwarded.");
        return;
    }
    Commit split = Commit::deserialize(load_object(split_id));
    auto split_blob = split.get_blobs_commit();
    auto head_blob = head.get_blobs_commit();
    auto given_blob = given.get_blobs_commit();
    bool flag = false;
    Stage_Area now_stage;
    std::set<std::string> all_files;
    for(auto &f : split_blob){
        all_files.insert(f.first);
    }
    for(auto &f : head_blob){
        all_files.insert(f.first);
    }
    for(auto &f : given_blob){
        all_files.insert(f.first);
    }
    for(auto &f_name : all_files){
        bool in_s = false;
        if(split_blob.find(f_name) != split_blob.end()){
            in_s = true;
        }
        bool in_h = false;
        if(head_blob.find(f_name) != head_blob.end()){
            in_h = true;
        }
        bool in_g = false;
        if(given_blob.find(f_name) != given_blob.end()){
            in_g = true;
        }
        std::string content_s = "";
        if(in_s == true){
            content_s = split_blob[f_name];
        }
        std::string content_h = "";
        if(in_h == true){
            content_h = head_blob[f_name];
        }
        std::string content_g = "";
        if(in_g == true){
            content_g = given_blob[f_name];
        }
        bool h_c = (in_s ? (content_s != content_h) : in_h);
        bool g_c = (in_s ? (content_s != content_g) : in_g);
        if(g_c == true && h_c == false){
            if(in_g){
                std::string content = load_object(content_g);
                Utils::writeContents(f_name,content);
                now_stage.add(f_name,content_g);
            }
            else {
                if(Utils::isFile(f_name)){
                    Utils::restrictedDelete(f_name);
                }
                now_stage.mark_remove(f_name);
            }
        }
        else {
            if(h_c == true && g_c == false){
                continue;
            }
            else {
                if(h_c == false && g_c == false){
                    continue;
                }
                else {
                    flag = true;
                    std::string head_c = in_h ? load_object(content_h) : "";
                    std::string given_c = in_g ? load_object(content_g) : "";
                    std::ostringstream merge;
                    merge<<"<<<<<<< HEAD\n";
                    merge<<head_c;
                    if(!head_c.empty() && head_c.back() != '\n'){
                        merge<<"\n";
                    }
                    merge<<"=======\n";
                    merge<<given_c;
                    if(!given_c.empty() && given_c.back() != '\n'){
                        merge<<"\n";
                    }
                    merge<<">>>>>>>\n";
                    std::string merge_str = merge.str();
                    Utils::writeContents(f_name,merge_str);
                    Blob b(f_name,merge_str);
                    save_blob(b);
                    now_stage.add(f_name,b.get_sha());
                }
            }
        }
        write_stage(now_stage);
        if(flag){
            Utils::message("Encountered a merge conflict.");
            return;
        }
        std::string branch = now;
        std::string former1 = head.get_id();
        std::string former2 = given.get_id();
        std::map<std::string,std::string> blob = head.get_blobs_commit();
        Commit merge_commit("Merged"+branch_name+"into"+branch,
                            std::vector<std::string>{former1,former2},blob,now_stage);
        save_object(merge_commit.get_id(),merge_commit.serialize());
        write_ref(branch,merge_commit.get_id());
        clear_stage();
    }
}





























