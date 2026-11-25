// Author: Claude（Claude が実装。README.md「実装者の記録」参照）
// 床と4枚の壁（と天井）を数値から自動で組み立てる部屋。扉や通路の穴は Openings で指定する。
// アクターの位置 = 床の上面の中心。壁は部屋の境界線の上に立つ（厚さの半分ずつ内側と外側）ので、
// RoomSize の間隔で並べると隣の部屋と壁の位置がぴったり重なる。重なる壁はどちらか片方を消す。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BallRoom.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

// 壁の面（アクターのローカル座標で見た向き）
UENUM(BlueprintType)
enum class ERoomSide : uint8
{
	PlusX  UMETA(DisplayName="+X（赤い矢印の先）"),
	MinusX UMETA(DisplayName="-X（赤い矢印の逆）"),
	PlusY  UMETA(DisplayName="+Y（緑の矢印の先）"),
	MinusY UMETA(DisplayName="-Y（緑の矢印の逆）"),
};

// 壁にあける穴（扉・通路用）
USTRUCT(BlueprintType)
struct FRoomOpening
{
	GENERATED_BODY()

	// どの面の壁にあけるか
	UPROPERTY(EditAnywhere, Category="Opening")
	ERoomSide Side = ERoomSide::PlusX;

	// 穴の中心の位置 (cm)。部屋の中心から、壁に沿った方向にずらす量
	// （+X/-X の壁なら Y 方向、+Y/-Y の壁なら X 方向）
	UPROPERTY(EditAnywhere, Category="Opening")
	float Offset = 0.f;

	// 穴の幅 (cm)。「あ」の部屋の入口（テンプレートの円柱の直径 500）に合わせてある
	UPROPERTY(EditAnywhere, Category="Opening", meta=(ClampMin="0.0"))
	float Width = 500.f;

	// オンなら Height の高さまで穴をあける。オフなら壁の上端まで全部抜ける
	UPROPERTY(EditAnywhere, Category="Opening", meta=(InlineEditConditionToggle))
	bool bUseHeight = false;

	// 穴の高さ (cm)。左のチェックをオンにしたときだけ使う。扉 BP_BallDoor の高さは 300
	UPROPERTY(EditAnywhere, Category="Opening", meta=(ClampMin="0.0", EditCondition="bUseHeight"))
	float Height = 300.f;

	// 穴の下端の高さ (cm)。床の上面からの高さ。0 なら床から
	UPROPERTY(EditAnywhere, Category="Opening", meta=(ClampMin="0.0"))
	float Bottom = 0.f;
};

UCLASS(PrioritizeCategories=("Room"), HideCategories=(Rendering, Replication, Collision, HLOD, Input, LOD, Cooking, Physics, Networking, Navigation, Tags, Activation, "Actor Tick"))
class BALLCONTROLLER_API ABallRoom : public AActor
{
	GENERATED_BODY()

public:
	ABallRoom();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	// ===== 寸法（既定値が全部屋の標準。変えたい部屋だけ詳細パネルで上書きする） =====

	// 部屋の大きさ (cm)。X = 赤い矢印方向、Y = 緑の矢印方向。壁の中心線どうしの距離
	UPROPERTY(EditAnywhere, Category="Room|Size")
	FVector2D RoomSize = FVector2D(4000.f, 4000.f);

	// 壁の高さ (cm)。床の上面から測る
	UPROPERTY(EditAnywhere, Category="Room|Size", meta=(ClampMin="0.0"))
	float WallHeight = 400.f;

	// 壁の厚さ (cm)
	UPROPERTY(EditAnywhere, Category="Room|Size", meta=(ClampMin="1.0"))
	float WallThickness = 100.f;

	// 床の厚さ (cm)
	UPROPERTY(EditAnywhere, Category="Room|Size", meta=(ClampMin="1.0"))
	float FloorThickness = 50.f;

	// 天井の厚さ (cm)
	UPROPERTY(EditAnywhere, Category="Room|Size", meta=(ClampMin="1.0"))
	float CeilingThickness = 50.f;

	// ===== どこを作るか =====

	UPROPERTY(EditAnywhere, Category="Room|Parts")
	bool bFloor = true;

	UPROPERTY(EditAnywhere, Category="Room|Parts")
	bool bCeiling = false;

	UPROPERTY(EditAnywhere, Category="Room|Parts")
	bool bWallPlusX = true;

	UPROPERTY(EditAnywhere, Category="Room|Parts")
	bool bWallMinusX = true;

	UPROPERTY(EditAnywhere, Category="Room|Parts")
	bool bWallPlusY = true;

	UPROPERTY(EditAnywhere, Category="Room|Parts")
	bool bWallMinusY = true;

	// 壁の穴。＋ボタンで増やす
	UPROPERTY(EditAnywhere, Category="Room|Openings")
	TArray<FRoomOpening> Openings;

	// ===== 見た目 =====

	UPROPERTY(EditAnywhere, Category="Room|Color")
	FLinearColor FloorColor = FLinearColor(0.35f, 0.35f, 0.35f);

	UPROPERTY(EditAnywhere, Category="Room|Color")
	FLinearColor WallColor = FLinearColor(0.6f, 0.6f, 0.6f);

	// 色を付ける元のマテリアル（"Color" ベクトルパラメータを持つもの）。空なら色は付かない
	UPROPERTY(EditAnywhere, Category="Room|Color")
	TObjectPtr<UMaterialInterface> BaseMaterial;

	// ===== コンポーネント =====

	// コンポーネントは数値から自動で作るので、詳細パネルには出さない
	// （出すと、コンポーネントの位置・メッシュ・コリジョンなどの項目が大量に並んで Room が下に埋もれる）
	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	// 床と天井
	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> FloorBoxes;

	// 壁
	UPROPERTY()
	TObjectPtr<UInstancedStaticMeshComponent> WallBoxes;

private:
	void Rebuild();

	// 1面ぶんの壁を、穴を避けて箱に分けて追加する
	void BuildWall(ERoomSide Side);

	// 中心とサイズ (cm) を指定して箱を1つ追加する（ローカル座標）
	static void AddBox(UInstancedStaticMeshComponent* ISM, const FVector& Center, const FVector& Size);

	void ApplyColor(UInstancedStaticMeshComponent* ISM, const FLinearColor& Color, TObjectPtr<UMaterialInstanceDynamic>& MID);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FloorMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> WallMaterial;
};
