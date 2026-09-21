#include "Character/LSSkillDataAsset.h"

ULSSkillDataAsset::ULSSkillDataAsset()
{

}

FPrimaryAssetId ULSSkillDataAsset::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(TEXT("Skill"), GetFName());
}