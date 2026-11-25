// Author: Claude（Claude が実装。README.md「実装者の記録」参照）
// 下り坂／上り坂の部屋。BallRoom の床を坂に差し替え、壁・穴・扉を坂の床の高さに合わせる。
// アクターの位置 = 始点（-X 側の壁の中心線上、床の上面）。坂は赤い矢印（+X）の方向に伸びる。
// 長さ（RoomSize.X）や傾斜を変えても始点は動かず、終点の側だけが伸び縮みする。
// 横の壁（±Y）は、上端と下端が坂と平行な斜めの形（ProceduralMesh）で作る。

#pragma once

#include "CoreMinimal.h"
#include "BallRoom.h"
#include "BallSlopeRoom.generated.h"

class UProceduralMeshComponent;

UENUM(BlueprintType)
enum class EBallSlopeDirection : uint8
{
	Down UMETA(DisplayName="下り坂"),
	Up   UMETA(DisplayName="上り坂"),
};

UCLASS(PrioritizeCategories=("Room"))
class BALLCONTROLLER_API ABallSlopeRoom : public ABallRoom
{
	GENERATED_BODY()

public:
	ABallSlopeRoom();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	// 坂の長さは「Room|Size」の RoomSize の X（水平に測った長さ）、幅は RoomSize の Y で決める

	// 下り坂か上り坂か（始点から +X 方向に進んだときの向き）
	UPROPERTY(EditAnywhere, Category="Room|Slope")
	EBallSlopeDirection Direction = EBallSlopeDirection::Down;

	// 傾斜の角度（度）。0 で平ら
	UPROPERTY(EditAnywhere, Category="Room|Slope", meta=(ClampMin="0.0", ClampMax="60.0", UIMin="0.0", UIMax="60.0"))
	float SlopeAngle = 10.f;

	// 終点（+X 側の壁の中心線上、床の上面）の、始点から見た位置 (cm)。次の部屋を置くときの目安。表示だけ
	UPROPERTY(VisibleAnywhere, Category="Room|Slope")
	FVector EndPoint = FVector::ZeroVector;

	virtual void BuildFloor() override;
	virtual void BuildCeiling() override;
	virtual float GetFloorZ(float LocalX) const override;
	virtual void AddWallPiece(bool bAlongX, float Normal, float U0, float U1, float ZFrom, float ZTo) override;

	// 横の壁の斜めの部分（数値から自動で作るので、詳細パネルには出さない）
	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> SideWalls;
	virtual float GetWallBottomDepth() const override;
	virtual FVector GetCenterOffset() const override;

private:
	// 高さ / 水平の長さ（上りは +、下りは -）
	float GetSlopeTan() const;

	// 部屋の中心から X の位置での、坂の面の高さ（両端で止めない）
	float SlopeZ(float LocalX) const;

	// X0〜X1, Y0〜Y1 の範囲に、坂と平行な板を置く。TopZOffset = 坂の面から板の上面までの高さ
	void AddSlopedSlab(float X0, float X1, float Y0, float Y1, float TopZOffset, float Thickness);

	// X0〜X1 の範囲に、上端と下端が坂と平行で両端が垂直な壁を1つ追加する
	void AddSlopedWall(float X0, float X1, float Normal, float ZFrom, float ZTo);

	// 面を1枚追加する（裏表どちらからも見えるように、両向きの三角形を入れる）
	void AddQuad(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, const FVector& Normal);

	// 組み立て中の横の壁のメッシュ
	TArray<FVector> WallVerts;
	TArray<int32> WallTris;
	TArray<FVector> WallNormals;
	TArray<FVector2D> WallUVs;
};
