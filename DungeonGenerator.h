// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonGenerator.generated.h"


struct FAStarNode
{
    FVector Position;  // Using FVector for grid positions
    float GCost;       // Cost from start node to this node
    float HCost;       // Heuristic cost from this node to end node
    float FCost() const { return GCost + HCost; }  // Total cost
    FAStarNode* CameFrom;  // Parent node in the path

    // Constructors
    FAStarNode(FVector Pos = FVector::ZeroVector, float G = FLT_MAX, float H = FLT_MAX, FAStarNode* Parent = nullptr)
        : Position(Pos), GCost(G), HCost(H), CameFrom(Parent) {}

    // Needed for sorting TArray
    bool operator<(const FAStarNode& Other) const
    {
        return FCost() < Other.FCost();
    }
};

USTRUCT()
struct FRoomConnection
{
    GENERATED_BODY()

public:
    UPROPERTY()
    int32 RoomIndexA;

    UPROPERTY()
    int32 RoomIndexB;

    UPROPERTY()
    float Distance;

    FRoomConnection(int32 a = 0, int32 b = 0, float dist = 0.0f) : RoomIndexA(a), RoomIndexB(b), Distance(dist) {}

    bool operator<(const FRoomConnection& Other) const {
        return Distance < Other.Distance;
    }
};


USTRUCT(BlueprintType)
struct FRoom
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 StartX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 StartY;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Width;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Height;

 UPROPERTY(EditAnywhere, BlueprintReadWrite)
     FVector EntryPoint;  // Entry point for pathfinding, typically at a room edge facing a corridor

	 UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector ExitPoint;   // Exit point for pathfinding, opposite or different from the entry point

    FRoom(int32 x = 0, int32 y = 0, int32 w = 1, int32 h = 1) 
        : StartX(x), StartY(y), Width(w), Height(h), 
          EntryPoint(FVector((x + (w - 1)/2), (y + (h-1)/2), 0)),  // Midpoint of the left wall
          ExitPoint(FVector((x + (w - 1)/2), (y + (h-1)/2), 0)) {}  // Midpoint of the right wall
};

UCLASS()
class REALONE_API ADungeonGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADungeonGenerator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, Category = "Config")
    TSubclassOf<AActor> WallClass;

    UPROPERTY(EditAnywhere, Category = "Config")
    TSubclassOf<AActor> FloorTileClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
    float CellSize = 100.0f;

	 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
    int32 Width = 30;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
    int32 Height = 30;

     UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
    int32 Length = 30;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
    int32 NumofRoom = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
    TArray<int32> Grid;
    

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dungeon")
	TArray<FRoom> Rooms;
	

	UPROPERTY(EditAnywhere, Category="Dungeon|Meshes")
    UStaticMesh* RoomMesh;

    UPROPERTY(EditAnywhere, Category="Dungeon|Meshes")
    UStaticMesh* CorridorMesh;

    UPROPERTY(EditAnywhere, Category="Dungeon|Meshes")
    UStaticMesh* DoorMesh;

    // Function to initialize the grid
    void InitializeGrid();

    // Function to place the initial room
    void PlaceInitialRoom();

	void DrawDebugGrid();

	void PlaceMultipleRooms(int32 NumberOfRooms);

	bool CanPlaceRoom(int32 X, int32 Y, int32 RoomWidth, int32 RoomHeight);

	void PlaceRoom(int32 X, int32 Y, int32 RoomWidth, int32 RoomHeight);

	int32 GetRoomIndex(int32 X, int32 Y);

	

	void FinalizeDungeon();

	void GenerateDungeon();

	void PlaceDoors();

	void PlaceMeshes();

	void GenerateAllRoomConnections(TArray<FRoomConnection>& OutConnections);

	void GenerateMinimumSpanningTree(TArray<FRoomConnection>& Connections, TArray<FRoomConnection>& OutMST);

	TArray<FRoomConnection> KruskalsMST();

	 int32 Find(int32 Index, TArray<int32>& Parent);

	 void Union(int32 IndexA, int32 IndexB, TArray<int32>& Parent, TArray<int32>& Rank);

	 void ConnectRoomsUsingAStar(const TArray<FRoomConnection>& MST);
	
	TArray<FAStarNode*> FindPath(const FVector& Start, const FVector& Goal);

	bool IsWalkable(const FVector& Position, const FVector& StartPos, const FVector& TargetPos);

	TArray<FVector> GetNeighbors(const FVector& NodePosition, const FVector& StartPos, const FVector& TargetPos);

	void DrawDebugRoomPoints();

    bool IsInRoom(const FVector& Position, const FVector& RoomPosition);

    int32 GetCorridorType(const FVector& Direction);

    void PlaceCorridor(const FVector& Position, int32 Type);

    void SpawnDungeonEnvironment();

    void SpawnCorridorWalls(int x, int y, int32 CorridorType);

    void SpawnWallTile(const FVector& Location, const FRotator& Rotation);

    void SpawnFloorTile(const FVector& Location);
};
