// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonGenerator.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMeshActor.h"
#include "Containers/Queue.h"



TArray<FVector> ADungeonGenerator::GetNeighbors(const FVector& NodePosition, const FVector& StartPos, const FVector& TargetPos)
{
    TArray<FVector> Neighbors;
    TArray<FVector> Directions = {
        FVector(1, 0, 0), FVector(-1, 0, 0), 
        FVector(0, 1, 0), FVector(0, -1, 0)
    };

    for (const FVector& Direction : Directions)
    {
        FVector NewPos = NodePosition + Direction;
        if (NewPos.X >= 0 && NewPos.X < Width  &&
            NewPos.Y >= 0 && NewPos.Y < Height  &&
            IsWalkable(NewPos, StartPos, TargetPos))  // Adjusted for new IsWalkable
        {
            Neighbors.Add(NewPos);
        }
    }

    return Neighbors;
}

bool ADungeonGenerator::IsWalkable(const FVector& Position, const FVector& StartPos, const FVector& TargetPos)
{
    int32 Col = Position.X;
    int32 Row = Position.Y;
    int32 Index = Row * Width + Col;

    if (!Grid.IsValidIndex(Index))
        return false;  // Ensure we are within the grid bounds

    // Check if the position is within the starting or target room
    bool isInStartRoom = IsInRoom(Position, StartPos);
    bool isInTargetRoom = IsInRoom(Position, TargetPos);

    return Grid[Index] == 0 || isInStartRoom || isInTargetRoom||Grid[Index]==2;  // Allow movement through empty spaces and through any room cells that are part of the start or target rooms
}

bool ADungeonGenerator::IsInRoom(const FVector& Position, const FVector& RoomPosition)
{
    for (const FRoom& Room : Rooms)
    {
        if (Position.X >= Room.StartX  && Position.X < (Room.StartX + Room.Width)  &&
            Position.Y >= Room.StartY  && Position.Y < (Room.StartY + Room.Height) )
        {
            return true;
        }
    }
    return false;
}


TArray<FAStarNode*> ADungeonGenerator::FindPath(const FVector& StartPos, const FVector& TargetPos)
{
    TArray<FAStarNode*> Path;
    TArray<FAStarNode*> OpenSet;
    TMap<FVector, FAStarNode*> AllNodes;  // No need for custom key comparators

    FAStarNode* StartNode = new FAStarNode(StartPos, 0, FVector::Dist(StartPos, TargetPos));
	
    OpenSet.Add(StartNode);
    AllNodes.Add(StartPos, StartNode);

    while (OpenSet.Num() > 0)
    {
        // Sort OpenSet based on FCost, then HCost for ties
        OpenSet.Sort([](const FAStarNode& A, const FAStarNode& B) {
            return A.FCost() == B.FCost() ? A.HCost < B.HCost : A.FCost() < B.FCost();
        });

        FAStarNode* CurrentNode = OpenSet[0];
        OpenSet.RemoveAt(0);  // Pop the first element (the node with the lowest F cost)
		
        if (CurrentNode->Position.Equals(TargetPos, 1.0f))  // Check proximity within a small threshold
        {
            while (CurrentNode != nullptr)
            {
                Path.Add(CurrentNode);
			
                CurrentNode = CurrentNode->CameFrom;
            }
            Algo::Reverse(Path);
            break;
        }

        TArray<FVector> Neighbors = GetNeighbors(CurrentNode->Position,StartPos, TargetPos);

		

        for (const FVector& Neighbor : Neighbors)
        {
			
          
			
            float TentativeGCost = CurrentNode->GCost + (CurrentNode->Position - Neighbor).Size();  // Distance as cost
            FAStarNode* NeighborNode = AllNodes.FindRef(Neighbor);
			
            if (!NeighborNode)
            {
                NeighborNode = new FAStarNode(Neighbor, TentativeGCost, FVector::Dist(Neighbor, TargetPos), CurrentNode);
                OpenSet.Add(NeighborNode);
                AllNodes.Add(Neighbor, NeighborNode);
            }
            else if (TentativeGCost < NeighborNode->GCost)
            {
                NeighborNode->CameFrom = CurrentNode;
                NeighborNode->GCost = TentativeGCost;
            }
        }
    }

    // Clean up
    /*for (const auto& NodePair : AllNodes)
    {
        if (!Path.Contains(NodePair.Value))
        {
            delete NodePair.Value;  // Delete nodes not in the final path to prevent memory leaks
        }
    }*/

    return Path;
}

int32 ADungeonGenerator::GetCorridorType(const FVector& Direction)
{
    if (Direction.X > 0) return 5;  // East
    if (Direction.X < 0) return 4;  // West
    if (Direction.Y > 0) return 3;  // South
    if (Direction.Y < 0) return 2;  // North
    return 0;
}

void ADungeonGenerator::PlaceCorridor(const FVector& Position, int32 Type)
{
    int32 X = Position.X ;
    int32 Y = Position.Y ;
    int32 Index = Y * Width + X;
    if (Grid.IsValidIndex(Index)&&Grid[Index]!=1)
        Grid[Index] = Type;
}

void ADungeonGenerator::ConnectRoomsUsingAStar(const TArray<FRoomConnection>& MST)
{
    for (const FRoomConnection& Connection : MST)
    {
        const FRoom& RoomA = Rooms[Connection.RoomIndexA];
        const FRoom& RoomB = Rooms[Connection.RoomIndexB];

        FVector StartPos = RoomA.EntryPoint;
        FVector TargetPos = RoomB.ExitPoint;

        TArray<FAStarNode*> Path = FindPath(StartPos, TargetPos);
         UE_LOG(LogTemp, Warning, TEXT("Path Generated between %d and %d"), Connection.RoomIndexA, Connection.RoomIndexB);
        FAStarNode* LastNode = nullptr;
        if (Path.Num() > 0)
        {
             
            for (FAStarNode* Node : Path)
            {
                if (LastNode != nullptr)
                {
                    FVector Direction = Node->Position - LastNode->Position;
                    int32 CorridorType = GetCorridorType(Direction);
                    PlaceCorridor(LastNode->Position, CorridorType);
                }
                LastNode = Node;
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("No path found between rooms %d and %d"), Connection.RoomIndexA, Connection.RoomIndexB);
        }
    }
}

void ADungeonGenerator::PlaceDoors()
{
    for (int32 i = 0; i < Rooms.Num(); i++)
    {
        const FRoom& room = Rooms[i];
        // Check potential door positions around the room perimeter
        for (int32 x = room.StartX; x < room.StartX + room.Width; x++) {
            // Top edge of the room
            if (room.StartY > 0 && Grid[(room.StartY - 1) * Width + x] == 2) {
                Grid[(room.StartY - 1) * Width + x] = 3;  // 3 for door
            }
            // Bottom edge of the room
            if (room.StartY + room.Height < Height && Grid[(room.StartY + room.Height) * Width + x] == 2) {
                Grid[(room.StartY + room.Height) * Width + x] = 3;  // 3 for door
            }
        }
        for (int32 y = room.StartY; y < room.StartY + room.Height; y++) {
            // Left edge of the room
            if (room.StartX > 0 && Grid[y * Width + (room.StartX - 1)] == 2) {
                Grid[y * Width + (room.StartX - 1)] = 3;  // 3 for door
            }
            // Right edge of the room
            if (room.StartX + room.Width < Width && Grid[y * Width + (room.StartX + room.Width)] == 2) {
                Grid[y * Width + (room.StartX + room.Width)] = 3;  // 3 for door
            }
        }
    }
}


int32 ADungeonGenerator::Find(int32 i, TArray<int32>& Parent)
{
    while (Parent[i] != i)
    {
        Parent[i] = Parent[Parent[i]];  // Path compression
        i = Parent[i];
    }
    return i;
}

void ADungeonGenerator::Union(int32 a, int32 b, TArray<int32>& Parent, TArray<int32>& Rank)
{
    int32 rootA = Find(a, Parent);
    int32 rootB = Find(b, Parent);
    if (rootA != rootB)
    {
        if (Rank[rootA] < Rank[rootB])
        {
            Parent[rootA] = rootB;
        }
        else if (Rank[rootA] > Rank[rootB])
        {
            Parent[rootB] = rootA;
        }
        else
        {
            Parent[rootB] = rootA;
            Rank[rootA]++;
        }
    }
}

TArray<FRoomConnection> ADungeonGenerator::KruskalsMST()
{
    TArray<FRoomConnection> AllConnections;
    GenerateAllRoomConnections(AllConnections);

    TArray<int32> Parent;
    TArray<int32> Rank;
    TArray<FRoomConnection> MST;

    for (int32 i = 0; i < Rooms.Num(); i++)
    {
        Parent.Add(i);
        Rank.Add(0);
    }

    for (const FRoomConnection& Conn : AllConnections)
    {
        if (Find(Conn.RoomIndexA, Parent) != Find(Conn.RoomIndexB, Parent))
        {
            MST.Add(Conn);
            Union(Conn.RoomIndexA, Conn.RoomIndexB, Parent, Rank);
            if (MST.Num() == Rooms.Num() - 1) break;
        }
    }

    if (MST.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("No corridors to connect: MST is empty"));
    }
	
    return MST;
}

// Sets default values
ADungeonGenerator::ADungeonGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
    static ConstructorHelpers::FClassFinder<AActor> WallBPClass(TEXT("/Game/PathToBP_Wall.BP_Wall_C"));
    if (WallBPClass.Class != NULL)
    {
        WallClass = WallBPClass.Class;
    }

}
void ADungeonGenerator::GenerateAllRoomConnections(TArray<FRoomConnection>& OutConnections)
{
    OutConnections.Empty();
    for (int32 i = 0; i < Rooms.Num(); i++)
    {
        for (int32 j = i + 1; j < Rooms.Num(); j++)
        {
            const FRoom& RoomA = Rooms[i];
            const FRoom& RoomB = Rooms[j];
            FVector CenterA(RoomA.StartX + RoomA.Width / 2.0f, RoomA.StartY + RoomA.Height / 2.0f, 0);
            FVector CenterB(RoomB.StartX + RoomB.Width / 2.0f, RoomB.StartY + RoomB.Height / 2.0f, 0);
            float Distance = FVector::Dist(CenterA, CenterB);
            OutConnections.Add(FRoomConnection(i, j, Distance));
        }
    }

    // Sort connections by distance
    OutConnections.Sort();
}

// Called when the game starts or when spawned
void ADungeonGenerator::BeginPlay()
{
	Super::BeginPlay();
	 GenerateDungeon();
}

// Called every frame
void ADungeonGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	 static int FrameCounter = 0;
    FrameCounter++;
    if (FrameCounter >= 400)
    {
        //DrawDebugGrid();
		
        FrameCounter = 0;  // reset counter after updating
    }
	
}

void ADungeonGenerator::GenerateDungeon()
{
  	 InitializeGrid();  // Set up the grid with default values
    PlaceMultipleRooms(NumofRoom);  // Place 10 rooms randomly

    TArray<FRoomConnection> MST = KruskalsMST();  // Generate the MST to find optimal room connections
    ConnectRoomsUsingAStar(MST);  // Connect rooms using corridors defined by A*

    SpawnDungeonEnvironment();  // Spawn the physical dungeon based on the grid
}

void ADungeonGenerator::SpawnFloorTile(const FVector& Location)
{
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = GetInstigator();
    FVector AdjustedLocation = Location - FVector(CellSize/2, CellSize/2, -CellSize/2);
    AActor* FloorActor = GetWorld()->SpawnActor<AActor>(FloorTileClass, AdjustedLocation, FRotator::ZeroRotator, SpawnParams);
    if (!FloorActor)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn floor tile at Location: %s"), *Location.ToString());
    }
}

void ADungeonGenerator::SpawnDungeonEnvironment()
{
    for (int32 y = 0; y < Height; y++)
    {
        for (int32 x = 0; x < Width; x++)
        {
            int32 Index = y * Width + x;
            FVector CellLocation = GetActorLocation() + FVector(x * CellSize, y * CellSize, 0);
            
            if (Grid[Index] == 1)  // Room
            {
                SpawnFloorTile(CellLocation);
            }
            else if (Grid[Index] >= 2 && Grid[Index] <= 5)  // Corridors
            {
                //SpawnCorridorTile(CellLocation, Grid[Index]);
                SpawnCorridorWalls(x, y, Grid[Index]);
            }
            else  // Walls/empty space
            {
                //SpawnWallTile(CellLocation);
            }
        }
    }
}

void ADungeonGenerator::SpawnWallTile(const FVector& Location, const FRotator& Rotation)
{
    // Assuming WallClass is a UClass* that points to BP_Wall
    if (!WallClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("Wall class not set."));
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = this;
    SpawnParams.Instigator = GetInstigator();

    // Adjust Location to be centered based on your grid cell size; assume CellSize is height of the wall
    FVector AdjustedLocation = Location + FVector(CellSize/2, CellSize/2, CellSize/2);

    // Spawn the wall
    AActor* WallActor = GetWorld()->SpawnActor<AActor>(WallClass, AdjustedLocation, Rotation, SpawnParams);
    if (!WallActor)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn wall at Location: %s"), *Location.ToString());
    }
}


void ADungeonGenerator::SpawnCorridorWalls(int x, int y, int32 CorridorType)
{
    FVector BaseLocation = GetActorLocation();
    FVector CellLocation = BaseLocation + FVector(x * CellSize, y * CellSize, 0);
    bool shouldSpawnEastWall = true, shouldSpawnWestWall = true, shouldSpawnNorthWall = true, shouldSpawnSouthWall = true;


    FRotator r1(0.0f, -180.0f, 0.0f);  // Vertical walls, faces East/West
    FRotator r2(0.0f, -90.0f, 0.0f);  
     FRotator r3(0.0f, -180.0f, 0.0f);  
     FRotator r4(0.0f, -90.0f, 0.0f);  
    // Check the grid boundaries and neighboring cells
    if (x > 0) // Check West
    {
        int32 WestIndex = y * Width + (x - 1);
        shouldSpawnWestWall = (Grid[WestIndex] == 0 ); // Empty or door
    }
    if (x < Width - 1) // Check East
    {
        int32 EastIndex = y * Width + (x + 1);
       
        shouldSpawnEastWall = (Grid[EastIndex] == 0 ); // Empty or door
    }
    if (y > 0) // Check North
    {
        int32 NorthIndex = (y - 1) * Width + x;
        shouldSpawnNorthWall = (Grid[NorthIndex] == 0 ); // Empty or door
    }
    if (y < Height - 1) // Check South
    {
        int32 SouthIndex = (y + 1) * Width + x;
        shouldSpawnSouthWall = (Grid[SouthIndex] == 0 ); // Empty or door
    }

    // Spawn walls where needed
    if (shouldSpawnEastWall)//done
    {
        FVector WallLocation = CellLocation;
        SpawnWallTile(WallLocation,r4);
    }
    if (shouldSpawnWestWall)//done
    {
        FVector WallLocation = CellLocation + FVector(-CellSize, 0, 0);
        SpawnWallTile(WallLocation,r2);
    }
    if (shouldSpawnNorthWall)//done
    {
        FVector WallLocation = CellLocation + FVector(0, -CellSize, 0);
        SpawnWallTile(WallLocation,r3);
    }
    if (shouldSpawnSouthWall)
    {
        FVector WallLocation = CellLocation;
        SpawnWallTile(WallLocation,r1);
    }

    SpawnFloorTile(CellLocation);
}


void ADungeonGenerator::InitializeGrid()
{
    Grid.SetNum(Width * Height);
    for (int32& Cell : Grid)
    {
        Cell = 0; // Initialize all grid cells to 0
    }
}

void ADungeonGenerator::PlaceMeshes()
{
    FVector Origin = GetActorLocation();
    float TileSize = 100.0f;  // Assuming each tile is 100x100 units

    for (int32 Y = 0; Y < Height; Y++)
    {
        for (int32 X = 0; X < Width; X++)
        {
            int32 Index = Y * Width + X;
            FVector Location = Origin + FVector(X * TileSize, Y * TileSize, 0);
            FRotator Rotation = FRotator(0, 0, 0);
            FActorSpawnParameters SpawnParams;

          if (Grid[Index] == 1 && RoomMesh)  // Check if the grid cell is a room
				{
					AStaticMeshActor* RoomActor = GetWorld()->SpawnActor<AStaticMeshActor>(Location, Rotation, SpawnParams);
					if (RoomActor)
					{
						RoomActor->GetStaticMeshComponent()->SetStaticMesh(RoomMesh);
					}
				}
				else if (Grid[Index] == 2 && CorridorMesh)  // Check if the grid cell is a corridor
				{
					AStaticMeshActor* CorridorActor = GetWorld()->SpawnActor<AStaticMeshActor>(Location, Rotation, SpawnParams);
					if (CorridorActor)
					{
						CorridorActor->GetStaticMeshComponent()->SetStaticMesh(CorridorMesh);
					}
				}
        }
    }
}

void ADungeonGenerator::DrawDebugGrid()
{
    FVector BaseLocation = GetActorLocation(); // This is the base location of the dungeon generator actor in the world.
     // Each cell is 100x100 units.

    FVector NorthLabelLocation = BaseLocation + FVector(Width * CellSize / 2, -CellSize, 50.0f);
    FVector SouthLabelLocation = BaseLocation + FVector(Width * CellSize / 2, Height * CellSize + CellSize, 50.0f);
    FVector EastLabelLocation = BaseLocation + FVector(Width * CellSize + CellSize, Height * CellSize / 2, 50.0f);
    FVector WestLabelLocation = BaseLocation + FVector(-CellSize, Height * CellSize / 2, 50.0f);

    DrawDebugString(GetWorld(), NorthLabelLocation, "N", nullptr, FColor::Red, -1.0f, true);
    DrawDebugString(GetWorld(), SouthLabelLocation, "S", nullptr, FColor::Red, -1.0f, true);
    DrawDebugString(GetWorld(), EastLabelLocation, "E", nullptr, FColor::Red, -1.0f, true);
    DrawDebugString(GetWorld(), WestLabelLocation, "W", nullptr, FColor::Red, -1.0f, true);
    
    for (int32 y = 0; y < Height; y++)
    {
        for (int32 x = 0; x < Width; x++)
        {
            int32 Index = y * Width + x;
            FVector CellLocation = BaseLocation + FVector(x * CellSize, y * CellSize, 0);  // Location of the current cell.
            FVector TextLocation = CellLocation + FVector(0, 0, 50);  // Slightly above the cell for visibility.

             FVector ArrowOrigin = CellLocation + FVector(0, 0, 10.0f);
            FVector ArrowDirection;
            float ArrowLength = 40.0f;
            FColor ArrowColor = FColor::Black;
            float ArrowThickness = 3.0f;

            if (Grid[Index] == 1)  // Room
            {
                DrawDebugBox(GetWorld(), CellLocation, FVector(CellSize/2, CellSize/2, 10.0f), FColor::Turquoise, true, -1, 0, 5);
                // Find which room this cell belongs to
                for (int32 i = 0; i < Rooms.Num(); i++)
                {
                    if (x >= Rooms[i].StartX && x < Rooms[i].StartX + Rooms[i].Width &&
                        y >= Rooms[i].StartY && y < Rooms[i].StartY + Rooms[i].Height)
                    {
                        DrawDebugString(GetWorld(), TextLocation, FString::Printf(TEXT("%d"), i), nullptr, FColor::White, -1, true);
                        break;
                    }
                }
            }
           else if (Grid[Index] == 2 || Grid[Index] == 3 || Grid[Index] == 4 || Grid[Index] == 5)  // Corridor with direction
            {
                DrawDebugBox(GetWorld(), CellLocation, FVector(CellSize/2, CellSize/2, 10.0f), FColor::Yellow, true, -1, 0, 5);
                switch (Grid[Index])
                {
                    case 2: // North
                        ArrowDirection = FVector(0, 1, 0);
                        break;
                    case 3: // South
                        ArrowDirection = FVector(0, -1, 0);
                        break;
                    case 4: // East
                        ArrowDirection = FVector(1, 0, 0);
                        break;
                    case 5: // West
                        ArrowDirection = FVector(-1, 0, 0);
                        break;
                }
                DrawDebugDirectionalArrow(GetWorld(), ArrowOrigin, ArrowOrigin + ArrowDirection * ArrowLength, ArrowLength, ArrowColor, true, -1, 0, ArrowThickness);
            }
            else if (Grid[Index] == 3)  // Door
            {
                DrawDebugBox(GetWorld(), CellLocation, FVector(CellSize/2, CellSize/2, 10.0f), FColor::Emerald, true, -1, 0, 5);
            }
            else  // Assume walls/empty space
            {
                DrawDebugBox(GetWorld(), CellLocation, FVector(CellSize/2, CellSize/2, 10.0f), FColor::Blue, true, -1, 0, 5);
            }
        }
    }

	// Highlight Entry and Exit Points
   
}

void ADungeonGenerator::PlaceMultipleRooms(int32 NumberOfRooms)
{
    int32 MaxAttempts = 100;
    for (int32 i = 0; i < NumberOfRooms; i++)
    {
        int32 Attempt = 0;
        bool bPlaced = false;
        while (!bPlaced && Attempt < MaxAttempts)
        {
            Attempt++;
            int32 RoomWidth = FMath::RandRange(4, 5);  // Randomize room width between 4 and 5
            int32 RoomHeight = FMath::RandRange(4, 5);  // Randomize room height between 4 and 5
            int32 StartX = FMath::RandRange(0, Width - RoomWidth);
            int32 StartY = FMath::RandRange(0, Height - RoomHeight);

            if (CanPlaceRoom(StartX, StartY, RoomWidth, RoomHeight))
            {
                PlaceRoom(StartX, StartY, RoomWidth, RoomHeight);
                bPlaced = true;
            }
        }
    }
}

bool ADungeonGenerator::CanPlaceRoom(int32 StartX, int32 StartY, int32 RoomWidth, int32 RoomHeight)
{
    for (int32 Y = StartY; Y < StartY + RoomHeight; Y++)
    {
        for (int32 X = StartX; X < StartX + RoomWidth; X++)
        {
            if (Grid[Y * Width + X] != 0)  // Check if the cell is already taken
            {
                return false;
            }
        }
    }
    return true;
}

void ADungeonGenerator::PlaceRoom(int32 StartX, int32 StartY, int32 RoomWidth, int32 RoomHeight)
{
    for (int32 Y = StartY; Y < StartY + RoomHeight; Y++)
    {
        for (int32 X = StartX; X < StartX + RoomWidth; X++)
        {
            Grid[Y * Width + X] = 1; // Set grid cells to 1 to indicate room presence
        }
    }

    // Add the room to the Rooms array
    Rooms.Add(FRoom(StartX, StartY, RoomWidth, RoomHeight));
}



void ADungeonGenerator::FinalizeDungeon()
{
    // Set the entry point
    Grid[0] = 3;  // 3 could signify an entry point

    // Set the exit point
    Grid[Width * Height - 1] = 4;  // 4 could signify an exit point

    // Potentially place other elements like traps (5) and treasure (6)
    for (const FRoom& Room : Rooms)
    {
        if (FMath::RandBool())  // Random chance to place a treasure
        {
            int32 TreasureX = FMath::RandRange(Room.StartX, Room.StartX + Room.Width - 1);
            int32 TreasureY = FMath::RandRange(Room.StartY, Room.StartY + Room.Height - 1);
            Grid[TreasureY * Width + TreasureX] = 6;
        }
    }
}

