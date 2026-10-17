#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include "canvas/BangVeCanvas.h"

struct ProjectItem {
    std::string id;
    std::string name;
    std::string ownerUsername;
    std::vector<std::string> members; // danh sach username duoc moi vao du an
    std::string createdAt;
    std::string updatedAt;
    std::uint64_t revision;
    std::unique_ptr<BangVeCanvas> canvas;

    ProjectItem();
};

struct ProjectSummary {
    std::string id;
    std::string name;
    std::string ownerUsername;
    std::vector<std::string> members;
    std::string createdAt;
    std::string updatedAt;
    bool isOwner;
};

class UserManager;

class ProjectManager {
private:
    std::string storageDir_;
    std::unordered_map<std::string, std::shared_ptr<ProjectItem>> projects_; // projectId -> ProjectItem
    mutable std::mutex mutex_;
    UserManager& userManager_;

    void loadAll();
    void saveAll();
    void saveProjectCanvas(const ProjectItem& item);
    void loadProjectCanvas(ProjectItem& item);

public:
    ProjectManager(UserManager& userManager, const std::string& storageDir = "output/projects");

    std::vector<ProjectSummary> getProjectsForUser(const std::string& username);

    std::shared_ptr<ProjectItem> getProject(const std::string& projectId);

    bool hasAccess(const std::string& projectId, const std::string& username);

    std::shared_ptr<ProjectItem> createProject(const std::string& name,
                                              const std::string& ownerUsername);

    bool inviteMember(const std::string& projectId,
                      const std::string& requester,
                      const std::string& targetUsername,
                      std::string& outError,
                      std::string* outAddedUsername = nullptr);

    bool removeMember(const std::string& projectId,
                      const std::string& requester,
                      const std::string& targetUsername,
                      std::string& outError);

    std::uint64_t updateProjectCanvas(const std::string& projectId,
                                     const std::string& username,
                                     const std::string& serializedCanvas,
                                     std::string& outError);
};
