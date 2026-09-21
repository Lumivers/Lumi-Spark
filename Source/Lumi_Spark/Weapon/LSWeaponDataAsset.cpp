#include "Weapon/LSWeaponDataAsset.h"

ULSWeaponDataAsset::ULSWeaponDataAsset()
{
    // 预设默认步枪后坐力模式序列（CS/Apex 风格前 6 发弹道）
    RecoilPattern.Add(FVector2D(-0.4f, 0.0f));
    RecoilPattern.Add(FVector2D(-0.6f, 0.05f));
    RecoilPattern.Add(FVector2D(-0.8f, -0.05f));
    RecoilPattern.Add(FVector2D(-0.9f, 0.12f));
    RecoilPattern.Add(FVector2D(-1.0f, -0.15f));
    RecoilPattern.Add(FVector2D(-1.1f, 0.08f));
}

FPrimaryAssetId ULSWeaponDataAsset::GetPrimaryAssetId() const
{
    return FPrimaryAssetId(TEXT("WeaponData"), GetFName());
}