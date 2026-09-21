#include "Character/LSCharacterDataAsset.h"

ULSCharacterDataAsset::ULSCharacterDataAsset()
{

}

FPrimaryAssetId ULSCharacterDataAsset::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(TEXT("CharacterData"), GetFName());
}