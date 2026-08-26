#include "LSEventBus.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

//静态查找当前世界的GameInstanceSubsystem
ULSEventBus* ULSEventBus::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject) return nullptr;
	
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World) return nullptr;
	
	if (const UGameInstance* GameInstance = World->GetGameInstance())
	{
		return GameInstance->GetSubsystem<ULSEventBus>();
	}
	
	return nullptr;
}