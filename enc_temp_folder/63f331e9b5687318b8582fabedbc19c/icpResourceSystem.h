#pragma once
#include "../core/icpMacros.h"
#include "icpResourceBase.h"
#include <map>
#include <queue>
#include <memory>
#include <TaskScheduler.h>

INCEPTION_BEGIN_NAMESPACE
class icpResourceSystem;

struct RunPinnedTaskLoopTask : public enki::IPinnedTask
{
	void Execute() override;
	icpResourceSystem* m_pResourceSystem = nullptr;
	enki::TaskScheduler* task_scheduler = nullptr;
	bool                    execute = true;
}; // struct RunPinnedTaskLoopTask

struct AsyncLoadFileResourceTask : public enki::IPinnedTask
{

	
	enki::TaskScheduler* task_scheduler = nullptr;
	bool m_bExecuting = true;
};

typedef std::map<icpResourceType, std::map<std::string, std::shared_ptr<icpResourceBase>>> icpResourceContainer;

struct ResourceLoadTask
{
	icpResourceType type;
	std::filesystem::path file_path;
};

class icpResourceSystem : public std::enable_shared_from_this<icpResourceSystem>
{
public:

	icpResourceSystem();

	~icpResourceSystem();

	bool Initialize();

	std::shared_ptr<icpResourceBase> loadImageResource(const std::filesystem::path& imgPath);
	std::shared_ptr<icpResourceBase> loadObjModelResource(const std::filesystem::path& objPath, bool ifLoadRelatedImgRes = false);

	bool LoadGLTFResource(const std::filesystem::path& gltfPath);

	bool RequestAsyncLoadResource(icpResourceType type, const std::filesystem::path& path);

	void UpdateSystem();

	icpResourceContainer& GetResourceContainer();

private:
	std::unique_ptr<enki::TaskScheduler> m_ekScheduler = nullptr;
	std::queue<ResourceLoadTask> m_taskLoadingQueue;

	RunPinnedTaskLoopTask m_run_pinned_task;
	icpResourceContainer m_resources;
};

INCEPTION_END_NAMESPACE