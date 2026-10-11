#include <filesystem>
#include <ios>
#include <iostream>

#include "FlowFieldMap.h"
#include "PlayerController.h"
#include "Map.h"
#include "Game.h"
#include "Unit.h"
#include "PointOfInterrest.h"

#include <queue>
#include <memory>


std::shared_ptr<PointOfInterrest> PlayerController::FindCheapestVillageUsingDijkstra(std::shared_ptr<Unit> unit, bool allowDiags = false)
{
	//This function selects the nearest Village from unit
	std::shared_ptr<Tile> startTile = game->GetMap()->GetTile(unit->GetPosition().x,unit->GetPosition().y);
	std::shared_ptr<Tile> currentTile = startTile;
	
	std::map <std::shared_ptr<Tile>, int> TileCost;
	TileCost[currentTile] = 0;
	
	std::map<std::shared_ptr<Tile>, int> lockedTiles;

	// register cost for each tile of the map
	while (currentTile != nullptr)
	{
	   	currentTile = nullptr;
		int minCost = 0x7FFFFFFF;
	
	   for (auto elem : TileCost)
	   {
			std::shared_ptr<Tile> tile = elem.first;
			int cost = elem.second;

		   	if (lockedTiles.count(tile) !=0) // already treated/locked with minimal cost
		   		continue;

		    if ( cost < minCost)
		    {
		   	    minCost = cost;
		   	    currentTile = tile;
		    }
	   }

	   		if (currentTile == nullptr)
	   			break;

	   		lockedTiles.insert({currentTile,currentTile->GetCost()});
	   		
	   		// --- ADJACENTS --

			// in order top, right, down, left
			std::vector dx2 =  {0,1,0,-1};
			std::vector dy2 =  {1,0,-1,0};
		
			if (allowDiags) // add diags in order, left top, right top, right down, left down
			{
				dx2.insert(dx2.end(), { -1, 1, 1,-1});
				dy2.insert(dy2.end(), { 1, 1, -1,-1});
			}

	   		// check directions
	   		for (int i = 0; i < dx2.size(); i++)
	   		{
	   			CVect nextPos = CVect(currentTile->GetPosition().x+dx2[i],currentTile->GetPosition().y+dy2[i]);
	   			std::shared_ptr<Tile> nextTile = game->GetMap()->GetTile(nextPos.x, nextPos.y);

	   			if (nextTile == nullptr || nextTile->GetCost()==0xFFFFFFFF) // doesn't exist or unpassable
	   				continue;

	   			if (lockedTiles.count(nextTile) != 0) // already treated/locked with minimal cost
	   				continue; 
	   			
	   			int newCost = minCost + nextTile->GetCost();
	   			auto it = TileCost.find(nextTile);

	   			// if cost isn't registered or is less than registered one => update cost for this tile
	   			if (it == TileCost.end()|| newCost < it->second) 
	   				TileCost[nextTile] = newCost;
	   		}
	   }
	
	// -- VILLAGE COMPARISON --
	
	int minCost =  0x7FFFFFFF;
	
	auto points = game->GetPointsOfInterest();
	std::shared_ptr<PointOfInterrest>  cheapestVillagePtr = nullptr;   

	// compare each village cost and register less cost one at each round
	for (auto point : points)
	{
	    if (point->GetEntityType() != EntityType::Village)
			continue;

		CVect pos = point->GetPosition();
		std::shared_ptr<Tile>  villageTile = game->GetMap()->GetTile(pos.x,pos.y) ;

		if (villageTile == nullptr) 
			continue;
		
		auto it = TileCost.find(villageTile);
		if (it != TileCost.end() && it->second < minCost)
		{
			minCost = it->second;
			cheapestVillagePtr = point;
		}
	}
	
	return cheapestVillagePtr;
}

void PlayerController::ComputePathUsingAStar(std::shared_ptr<Unit> unit, CVect to, bool allowDiags = false)
{
	//This function computes the path from a unit to a destination using A*
	
	std::shared_ptr<Tile> startTile = game->GetMap()->GetTile(unit->GetPosition().x,unit->GetPosition().y);
	std::shared_ptr<Tile> endTile = game->GetMap()->GetTile(to.x,to.y);
	
	std::shared_ptr<Tile> currentTile = startTile;
	
	std::map <std::shared_ptr<Tile>, std::pair<std::shared_ptr<Tile>,int>> TileCost;
	TileCost[currentTile] = std::pair(nullptr,0);
	
	std::map<std::shared_ptr<Tile>, int> lockedTiles;

	// register cost for each tile from start to end
   while (currentTile !=nullptr && currentTile != endTile)
   {
   		currentTile = nullptr;
		int minCost = 0x7FFFFFFF;
	
	   for (auto elem : TileCost)
	   {
	   		std::shared_ptr<Tile> tile = elem.first;
	   		int cost = elem.second.second;
	   	
		   	if (lockedTiles.count(tile) !=0) // already treated/locked with minimal cost
		   		continue;

	   		// convert djikstra in astar thx to heuristique val
	   		int heuristique = static_cast<int>((tile->GetPosition() - endTile->GetPosition()).length());
		    if (cost + heuristique < minCost)
		    {
		   	    currentTile = tile;
		   	    minCost = cost + heuristique;
		    }
	   }
		if (currentTile == nullptr || currentTile == endTile)
			break;
   	
	   	lockedTiles.insert({currentTile, minCost});
	   	
	   	// --- ADJACENTS --

   		// in order top, right, down, left
   		std::vector dx2 =  {0,1, 0,-1};
   		std::vector dy2 =  {1,0,-1, 0};
   	
   		if (allowDiags) // add diags in order, left top, right top, right down, left down
   		{
   			dx2.insert(dx2.end(), {-1, 1, 1,-1});
   			dy2.insert(dy2.end(), { 1, 1,-1,-1});
   		}

	   	// check directions
	   	for (int i = 0; i < dx2.size(); i++)
	   	{
	   		CVect nextPos = CVect(currentTile->GetPosition().x+dx2[i],currentTile->GetPosition().y+dy2[i]);
	   		std::shared_ptr<Tile> nextTile = game->GetMap()->GetTile(nextPos.x, nextPos.y);

	   		if (nextTile == nullptr || nextTile->GetCost()==0xFFFFFFFF) // doesn't exist or unpassable
	   			continue;

	   		if (lockedTiles.count(nextTile) !=0) // already treated/locked with minimal cost
	   			continue; 

	   		int currentCost = TileCost[currentTile].second;
	   		int newCost = currentCost + nextTile->GetCost();
	   		
	   		auto it = TileCost.find(nextTile);

	   		// if cost isn't registered or is less than registered one => update cost for this tile
	   		if (it == TileCost.end()|| newCost < it->second.second)
	   			TileCost[nextTile] = {currentTile, newCost};
	   	}
   }

	// -- PATH --
	std::vector<CVect> path;
	std::shared_ptr<Tile> currentTilePath = endTile;
	while (currentTilePath != startTile)
	{
		path.push_back(currentTilePath->GetPosition());

		auto it = TileCost.find(currentTilePath);
		if (it == TileCost.end() || it->second.first == nullptr)
			return;
		
		currentTilePath = it->second.first; 
	}
	std::reverse(path.begin(), path.end());

	unit->SetPath(path);
}

void FlowFieldMap::GenerateFlowField(std::shared_ptr<Game> game, CVect cvect, bool useCamps = false, bool allowDiags = false)
{
	//This function generates the flow field to get to the right destination:

	// Clean map
	auto map = game->GetMap();
	ResetMap(map->GetWidth(), map->GetHeight());

	//if useCamps = false => use cvect
	//if useCamps = true => use game to get the first camp
	CVect startPos = cvect;
	if (useCamps)
	{
		for (auto point : game->GetPointsOfInterest())
		{
			if (point->GetEntityType()==EntityType::GoblinCamp)
			{
				startPos = CVect(point->GetPosition().x,point->GetPosition().y); // get first camp by default
				break;
			}
		}
	}

	// -- FLOW FIELD --
	std::shared_ptr<Tile> startTile = game->GetMap()->GetTile(startPos.x,startPos.y);
	std::shared_ptr<Tile> currentTile = startTile;

	if (startTile == nullptr) return;

	std::map<std::shared_ptr<Tile>, int> lockedTiles;
	
	std::map <std::shared_ptr<Tile>, int> TileCost;
	TileCost[currentTile] = 0;
	
	distanceMap[startPos.y][startPos.x] = 0;

	// register cost for each tile of the map
	while (currentTile != nullptr)
	{
		std::shared_ptr<Tile> currentTile = nullptr;
		int minCost = 0x7FFFFFFF;
	
		for (auto elem : TileCost)
		{
			std::shared_ptr<Tile> tile = elem.first;
			int cost = elem.second;

			if (lockedTiles.count(tile) !=0) // already treated/locked with minimal cost
				continue;

			if ( cost < minCost)
			{
				minCost = cost;
				currentTile = tile;
			}
		}

		if (currentTile == nullptr)
			break;

		lockedTiles.insert({currentTile,currentTile->GetCost()});
	   		
		// --- ADJACENTS --
		
		// in order top, right, down, left
		std::vector dx2 =  {0,1,0,-1};
		std::vector dy2 =  {1,0,-1,0};
		
		if (allowDiags) // add diags in order, left top, right top, right down, left down
		{
			dx2.insert(dx2.end(), { -1, 1, 1,-1});
			dy2.insert(dy2.end(), { 1, 1, -1,-1});
		}

		// check directions
		for (int i = 0; i < dx2.size(); i++)
		{
			CVect nextPos = CVect(currentTile->GetPosition().x+dx2[i],currentTile->GetPosition().y+dy2[i]);
			std::shared_ptr<Tile> nextTile = game->GetMap()->GetTile(nextPos.x, nextPos.y);

			int indexI = nextPos.y;
			int indexJ = nextPos.x;
			
			if (nextTile == nullptr || nextTile->GetCost()==0xFFFFFFFF) // doesn't exist or unpassable
				continue;

			if (lockedTiles.count(nextTile) != 0)
				continue; // 
	   			
			int newCost = minCost + nextTile->GetCost();
			auto it = TileCost.find(nextTile);

			// if cost isn't registered or is less than registered one
			// => update cost for this tile
			// => update integration map + flow field
			if (it == TileCost.end()|| newCost < it->second)
			{
				TileCost[nextTile] = newCost;
				
				distanceMap[indexI][indexJ] = newCost;

				// update nexTile direction for flow field
				CVect dir = currentTile->GetPosition() - nextTile->GetPosition(); // from next to previous tile
				dir.normalize();
				flowField[indexI][indexJ] = dir;
			}
		}
	}

	
}