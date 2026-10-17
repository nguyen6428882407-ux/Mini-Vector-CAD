#include "auth/ProjectManager.h"
#include "auth/UserManager.h"
#include "io/TextSerializer.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <random>
#include <algorithm>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace {

std::string getCurrentTimeStr() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buf);
}

std::string generateProjectId() {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    static std::mt19937 gen(1337);
    std::uniform_int_distribution<int> dis(1000, 9999);
    return "proj_" + std::to_string(now % 1000000000) + "_" + std::to_string(dis(gen));
}

std::string joinMembers(const std::vector<std::string>& members) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < members.size(); ++i) {
        oss << members[i];
        if (i + 1 < members.size()) oss << ",";
    }
    return oss.str();
}

std::vector<std::string> splitMembers(const std::string& str) {
    std::vector<std::string> res;
    if (str.empty()) return res;
    std::istringstream iss(str);
    std::string item;
    while (std::getline(iss, item, ',')) {
        if (!item.empty()) res.push_back(item);
    }
    return res;
}

std::string escapeMeta(const std::string& s) {
    std::string res;
    for (char c : s) {
        if (c == '\t') res += "\\t";
        else if (c == '\n') res += "\\n";
        else if (c == '\\') res += "\\\\";
        else res += c;
    }
    return res;
}

std::string unescapeMeta(const std::string& s) {
    std::string res;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            if (s[i + 1] == 't') { res += '\t'; ++i; }
            else if (s[i + 1] == 'n') { res += '\n'; ++i; }
            else if (s[i + 1] == '\\') { res += '\\'; ++i; }
            else res += s[i];
        } else {
            res += s[i];
        }
    }
    return res;
}

} // namespace

ProjectItem::ProjectItem()
    : revision(1),
      canvas(std::make_unique<BangVeCanvas>(800.0, 600.0)) {
    canvas->themLayer("Layer 1");
}

ProjectManager::ProjectManager(UserManager& userManager, const std::string& storageDir)
    : storageDir_(storageDir), userManager_(userManager) {
    try {
        fs::create_directories(storageDir_);
    } catch (...) {}
    loadAll();
}

void ProjectManager::loadProjectCanvas(ProjectItem& item) {
    std::string canvasPath = storageDir_ + "/" + item.id + ".txt";
    if (fs::exists(canvasPath)) {
        auto loadedCanvas = std::make_unique<BangVeCanvas>(800.0, 600.0);
        if (TextSerializer::docFile(*loadedCanvas, canvasPath) && loadedCanvas->soLayer() > 0) {
            item.canvas = std::move(loadedCanvas);
        }
    }
}

void ProjectManager::saveProjectCanvas(const ProjectItem& item) {
    if (!item.canvas) return;
    std::string canvasPath = storageDir_ + "/" + item.id + ".txt";
    TextSerializer::ghiFile(*item.canvas, canvasPath);
}

void ProjectManager::loadAll() {
    std::string metaPath = storageDir_ + "/projects.txt";
    if (!fs::exists(metaPath)) {
        // Tạo dự án mặc định nếu chưa có dự án nào
        auto defProj = std::make_shared<ProjectItem>();
        defProj->id = "proj_default";
        defProj->name = "Dự án 1";
        defProj->ownerUsername = "hethong";
        defProj->createdAt = getCurrentTimeStr();
        defProj->updatedAt = defProj->createdAt;
        projects_[defProj->id] = defProj;
        saveAll();
        return;
    }

    std::ifstream ifs(metaPath);
    if (!ifs.is_open()) return;

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string id, name, owner, membersStr, createdAt, updatedAt, revStr;
        if (std::getline(iss, id, '\t') &&
            std::getline(iss, name, '\t') &&
            std::getline(iss, owner, '\t') &&
            std::getline(iss, membersStr, '\t') &&
            std::getline(iss, createdAt, '\t') &&
            std::getline(iss, updatedAt, '\t') &&
            std::getline(iss, revStr)) {
            auto item = std::make_shared<ProjectItem>();
            item->id = unescapeMeta(id);
            item->name = unescapeMeta(name);
            item->ownerUsername = unescapeMeta(owner);
            item->members = splitMembers(unescapeMeta(membersStr));
            item->createdAt = unescapeMeta(createdAt);
            item->updatedAt = unescapeMeta(updatedAt);
            try {
                item->revision = std::stoull(revStr);
            } catch (...) {
                item->revision = 1;
            }
            loadProjectCanvas(*item);
            projects_[item->id] = item;
        }
    }

    if (projects_.empty()) {
        auto defProj = std::make_shared<ProjectItem>();
        defProj->id = "proj_default";
        defProj->name = "Dự án 1";
        defProj->ownerUsername = "hethong";
        defProj->createdAt = getCurrentTimeStr();
        defProj->updatedAt = defProj->createdAt;
        projects_[defProj->id] = defProj;
        saveAll();
    }
}

void ProjectManager::saveAll() {
    std::string metaPath = storageDir_ + "/projects.txt";
    std::ofstream ofs(metaPath, std::ios::trunc);
    if (!ofs.is_open()) return;

    ofs << "# MINI_VECTOR_CAD_PROJECTS_V1\n";
    ofs << "# id\tname\towner\tmembers\tcreatedAt\tupdatedAt\trevision\n";
    for (const auto& pair : projects_) {
        const auto& p = *pair.second;
        ofs << escapeMeta(p.id) << "\t"
            << escapeMeta(p.name) << "\t"
            << escapeMeta(p.ownerUsername) << "\t"
            << escapeMeta(joinMembers(p.members)) << "\t"
            << escapeMeta(p.createdAt) << "\t"
            << escapeMeta(p.updatedAt) << "\t"
            << p.revision << "\n";
        saveProjectCanvas(p);
    }
}

std::vector<ProjectSummary> ProjectManager::getProjectsForUser(const std::string& username) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ProjectSummary> res;

    // Khách chưa đăng nhập: Không có dự án trên server (vẽ riêng cục bộ trên máy)
    if (username.empty()) {
        return res;
    }

    // Kiểm tra xem người dùng đã có dự án nào chưa
    bool hasAny = false;
    for (const auto& pair : projects_) {
        const auto& p = *pair.second;
        if (p.ownerUsername == username) {
            hasAny = true;
            break;
        }
        for (const auto& m : p.members) {
            if (m == username) {
                hasAny = true;
                break;
            }
        }
    }

    // Nếu người dùng mới chưa có dự án nào, tự động tạo 1 dự án riêng cá nhân
    if (!hasAny) {
        auto personalProj = std::make_shared<ProjectItem>();
        personalProj->id = generateProjectId();
        personalProj->name = "Bản vẽ của " + username;
        personalProj->ownerUsername = username;
        personalProj->createdAt = getCurrentTimeStr();
        personalProj->updatedAt = personalProj->createdAt;
        personalProj->revision = 1;
        projects_[personalProj->id] = personalProj;
        saveAll();
    }

    for (const auto& pair : projects_) {
        const auto& p = *pair.second;
        bool isOwner = (p.ownerUsername == username);
        bool isMember = false;
        for (const auto& m : p.members) {
            if (m == username) {
                isMember = true;
                break;
            }
        }

        // Chỉ hiển thị: dự án do mình làm chủ HOẶC dự án mình được mời vào vẽ chung
        if (isOwner || isMember) {
            ProjectSummary summary;
            summary.id = p.id;
            summary.name = p.name;
            summary.ownerUsername = p.ownerUsername;
            summary.members = p.members;
            summary.createdAt = p.createdAt;
            summary.updatedAt = p.updatedAt;
            summary.isOwner = isOwner;
            res.push_back(summary);
        }
    }

    // Sắp xếp: Dự án làm chủ lên trước, sau đó là dự án được mời
    std::sort(res.begin(), res.end(), [](const ProjectSummary& a, const ProjectSummary& b) {
        if (a.isOwner != b.isOwner) return a.isOwner > b.isOwner;
        return a.updatedAt > b.updatedAt;
    });

    return res;
}

std::shared_ptr<ProjectItem> ProjectManager::getProject(const std::string& projectId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = projects_.find(projectId);
    if (it != projects_.end()) return it->second;
    return nullptr;
}

bool ProjectManager::hasAccess(const std::string& projectId, const std::string& username) {
    if (username.empty()) return false; // Khách không có quyền truy cập dự án trên server
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = projects_.find(projectId);
    if (it == projects_.end()) return false;
    const auto& p = *it->second;
    if (p.ownerUsername == username) return true;
    for (const auto& m : p.members) {
        if (m == username) return true;
    }
    return false;
}

std::shared_ptr<ProjectItem> ProjectManager::createProject(const std::string& name,
                                                         const std::string& ownerUsername) {
    if (ownerUsername.empty()) return nullptr; // Khách không tạo dự án trên server
    std::lock_guard<std::mutex> lock(mutex_);
    auto item = std::make_shared<ProjectItem>();
    item->id = generateProjectId();
    if (name.empty()) {
        std::size_t nextNum = projects_.size() + 1;
        item->name = "Dự án " + std::to_string(nextNum);
    } else {
        item->name = name;
    }
    item->ownerUsername = ownerUsername;
    item->createdAt = getCurrentTimeStr();
    item->updatedAt = item->createdAt;
    item->revision = 1;

    projects_[item->id] = item;
    saveAll();
    return item;
}

bool ProjectManager::inviteMember(const std::string& projectId,
                                 const std::string& requester,
                                 const std::string& targetInput,
                                 std::string& outError,
                                 std::string* outAddedUsername) {
    if (requester.empty()) {
        outError = "Vui lòng đăng nhập để mời thành viên vẽ chung.";
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = projects_.find(projectId);
    if (it == projects_.end()) {
        outError = "Không tìm thấy dự án.";
        return false;
    }

    auto& p = *it->second;

    // Chỉ chủ sở hữu dự án mới có quyền mời thành viên khác vào vẽ chung
    if (p.ownerUsername != requester) {
        outError = "Chỉ chủ dự án mới có quyền mời thành viên vẽ chung.";
        return false;
    }

    if (targetInput.empty()) {
        outError = "Vui lòng nhập tên tài khoản hoặc họ tên người muốn mời.";
        return false;
    }

    // Tìm tài khoản không phân biệt hoa thường
    UserAccount targetUser;
    if (!userManager_.findUser(targetInput, targetUser)) {
        outError = "Không tìm thấy tài khoản hoặc người dùng \"" + targetInput + "\" trong hệ thống.";
        return false;
    }

    std::string targetUsername = targetUser.username;
    if (outAddedUsername) {
        *outAddedUsername = targetUsername;
    }

    if (p.ownerUsername == targetUsername) {
        outError = "Tài khoản \"" + targetUsername + "\" là chủ của dự án này rồi.";
        return false;
    }

    for (const auto& m : p.members) {
        if (m == targetUsername) {
            outError = "Tài khoản \"" + targetUsername + "\" đã là thành viên của dự án.";
            return false;
        }
    }

    p.members.push_back(targetUsername);
    p.updatedAt = getCurrentTimeStr();
    saveAll();
    return true;
}

bool ProjectManager::removeMember(const std::string& projectId,
                                 const std::string& requester,
                                 const std::string& targetUsername,
                                 std::string& outError) {
    if (requester.empty()) {
        outError = "Vui lòng đăng nhập.";
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = projects_.find(projectId);
    if (it == projects_.end()) {
        outError = "Không tìm thấy dự án.";
        return false;
    }

    auto& p = *it->second;
    bool isOwner = (p.ownerUsername == requester);
    bool isSelf = (requester == targetUsername);

    if (!isOwner && !isSelf) {
        outError = "Chỉ chủ dự án mới có quyền xóa thành viên khác.";
        return false;
    }

    auto memberIt = std::find(p.members.begin(), p.members.end(), targetUsername);
    if (memberIt == p.members.end()) {
        outError = "Không tìm thấy thành viên này trong dự án.";
        return false;
    }

    p.members.erase(memberIt);
    p.updatedAt = getCurrentTimeStr();
    saveAll();
    return true;
}

std::uint64_t ProjectManager::updateProjectCanvas(const std::string& projectId,
                                                const std::string& username,
                                                const std::string& serializedCanvas,
                                                std::string& outError) {
    if (username.empty()) {
        outError = "Tài khoản khách vẽ ở chế độ cục bộ riêng tư, không lưu lên server.";
        return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = projects_.find(projectId);
    if (it == projects_.end()) {
        outError = "Không tìm thấy dự án.";
        return 0;
    }

    auto& p = *it->second;
    bool canEdit = (p.ownerUsername == username);
    if (!canEdit) {
        for (const auto& m : p.members) {
            if (m == username) { canEdit = true; break; }
        }
    }
    if (!canEdit) {
        outError = "Bạn không có quyền chỉnh sửa dự án này.";
        return 0;
    }

    std::istringstream iss(serializedCanvas);
    auto staged = std::make_unique<BangVeCanvas>(800.0, 600.0);
    TextSerializer::deserialize(*staged, iss);
    if (iss.fail() || staged->soLayer() == 0) {
        outError = "Dữ liệu bản vẽ không hợp lệ.";
        return 0;
    }

    p.canvas = std::move(staged);
    p.updatedAt = getCurrentTimeStr();
    ++p.revision;
    saveProjectCanvas(p);
    return p.revision;
}
