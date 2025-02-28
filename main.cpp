#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif


#include<stdio.h>
#include <stdlib.h>
#include"conio2.h"






#define boardOffsetX 48
#define boardOffsetY 3

#define legendOffsetX 2
#define legendOffsetY 2

#define cursorBackgroundColor 9
#define borderSymbol 'X'
#define gameBoardSpacesStyle 1
#define borderStyle 2
#define defaultStyle 3

#define BACKSPACE 0x08
#define NULLCHR 0

#define whiteStone 1
#define blackStone 2

#define borderColor WHITE
#define gameBoardSpacesColor YELLOW
#define borderBackgroundColor BROWN
#define gameBoardSpacesBackgroundColor DARKGRAY

#define atariWarning RED

#define ESC 27
#define ENTER 13

#define UParrow 0x48
#define DOWNarrow 0x50
#define LEFTarrow 0x4b
#define RIGHTarrow 0x4d

//class which holds individual chars in one board row
class row
{
	public:
		char* tab = NULL;
		row(int rowSize)
		{
			tab = (char*)calloc(rowSize, sizeof(char));
			for (int i = 0;i < rowSize;i++)
			{
				tab[i] = 0;
			}
		};
		
		void destroy()
		{
			free(tab);
		}
};

//class which holds 2d positions
class position
{
public:
	int x=0;
	int y=0;

	position(int whatX, int whatY)
	{
		x = whatX;
		y = whatY;
	};
	position()
	{
		x = 0;
		y = 0;
	}

};

//class which holds the ids of stones in one stoneChain
class stoneChain
{
public:

	int* ids = NULL;
	int size=0;
	

	char color=0;

	//these two are only used for ending the game
	int* emptyGroupIds;
	int emptyGroupIdSize=0;

	void addStone(int id)
	{
		size++;
		ids = (int*)realloc(ids, size * sizeof(int));
		ids[size - 1] = id;
	};
	//combines two stoneChains into one big stoneChain.
	void combine(stoneChain b)
	{
		int prevSize = size;
		size = size + b.size;
		ids = (int*)realloc(ids, size * sizeof(int));

		
		for (int i = 0;i < b.size;i++)
		{
			ids[prevSize + i] = b.ids[i];
		}
	}

	
	void resetEmptyGroupIds()
	{
		free(emptyGroupIds);

		emptyGroupIdSize = 0;
		emptyGroupIds = NULL;
	}

	bool notInEmptyGroupIds(int id)
	{
		for (int i = 0;i < emptyGroupIdSize;i++)
		{
			if (emptyGroupIds[i] == id)
			{
				return false;
			}
		}
		return true;
	}

	void addEmptyGroupId(int id)
	{

		if (notInEmptyGroupIds(id))
		{
			emptyGroupIdSize++;
			
			emptyGroupIds = (int*)realloc(emptyGroupIds, emptyGroupIdSize * sizeof(int));
			emptyGroupIds[emptyGroupIdSize - 1] = id;
		}
				
	}
	stoneChain()
	{
		ids = NULL;
		size = 0;

		emptyGroupIds = NULL;
		emptyGroupIdSize = 0;
		
	}
	
	void destroy()
	{

		free(ids);
		free(emptyGroupIds);
	
	}
};
//prints whatever const char array is provided at the new lowest legend Y position,
//then increments it.
void printInLegend(const char str[], int* legendY)
{
	gotoxy(legendOffsetX, legendOffsetY + *legendY);
	cputs(str);
	*legendY = *legendY + 1;
}

//stores a dynamic array of positions
class positionVector
{
public:
	int size = 0;
	position* tab = NULL;

	positionVector()
	{
		tab = NULL;
		size = 0;
	};

	void add(position pos)
	{
		size++;
		tab = (position*)realloc(tab, sizeof(position) * size);
		tab[size - 1] = pos;
	};

	void remove(position pos)
	{
		int id = find(pos);
		if (id == -1)
		{
			//error
		}
		else
		{
			position tmp = tab[size - 1];
			tab[size - 1] = pos;
			tab[id] = tmp;

			size--;
			tab = (position*)realloc(tab, sizeof(position) * size);

		}
	}

	void shift()
	{
		position first = tab[0];
		position last = tab[size - 1];

		tab[size - 1] = first;
		tab[0] = last;

		size--;
		tab = (position*)realloc(tab, sizeof(position) * size);

	}
	int find(position pos)
	{
		for (int i = 0;i < size;i++)
		{
			if (tab[i].x == pos.x && tab[i].y == pos.y)
			{
				return i;
			}
		}
		return -1;
	};
	void destroy()
	{
		free(tab);
	}
};

//used when ending the game, contains connected empty spots
class emptyGroup
{
public:
	int size;
	position* tab = NULL;

	int touchesWhite = 0;
	int touchesBlack = 0;

	emptyGroup()
	{
		size = 0;
		tab = NULL;
	}

	int find(position pos)
	{
		for (int i = 0;i < size;i++)
		{
			if (tab[i].x == pos.x && tab[i].y == pos.y)
			{
				return i;
			}
		}
		return -1;
	}

	void add(position pos)
	{
		if (find(pos) == -1)
		{
			
			size++;
			tab = (position*)realloc(tab, size * sizeof(position));

			tab[size - 1] = pos;
		}
	}

	void destroy()
	{
	
		free(tab);
		
	}
};

//class containing most of the game logic and variables needed,
//holds information about the current state of the board and the game

class board
{


	public:
	row* tab;

	stoneChain* stoneChains = NULL;
	int stoneChainsSize = 0;

	int columnSize = 0;
	int rowSize = 0;

	position nearbyPos[4] = { position(-1,-1), position(-1,-1), position(-1,-1), position(-1,-1) };

	int playerStone = whiteStone;

	float whitePoints = 6.5;
	float blackPoints = 0;

	bool setupMode = false;

	int lastTakenIDWhite = -1;
	int lastTakenIDBlack = -1;

	position cursorOffset = position(0, 0);
	int chunkSizeX = 0;
	int chunkSizeY = 0;

	int territoriesWhite = 0;
	int territoriesBlack = 0;

	bool gameEnded = false;

	//dynamic arrays below are used for game end logic

	emptyGroup* emptyGroups = NULL;
	int emptyGroupSize = 0;

	int* chainsToIgnoreForNow = NULL;
	int chainsToIgnoreForNowSize = 0;

	board(int cs, int rs)
	{
		columnSize = cs;
		rowSize = rs;

		tab = (row*)calloc(columnSize, sizeof(row));

		stoneChains = NULL;
		stoneChainsSize = 0;

		emptyGroups = NULL;
		emptyGroupSize = 0;

		chainsToIgnoreForNow = NULL;
		chainsToIgnoreForNowSize = 0;

		for (int i = 0;i < columnSize;i++)
		{
			tab[i] = row(rowSize);
		}
		

	};

	
	void addNewStoneChain()
	{
		stoneChainsSize++;
		stoneChains = (stoneChain*)realloc(stoneChains, stoneChainsSize * sizeof(stoneChain));
		stoneChains[stoneChainsSize - 1] = stoneChain();

	};

	//checks if a position is inside board
	bool inBoard(position pos)
	{
		if (pos.x >= 0 && pos.x < columnSize && pos.y >= 0 && pos.y < rowSize)
		{
				return true;
		}
		return false;
	};

	bool isEmpty(position pos)
	{
		if (tab[pos.x].tab[pos.y] == 0)
		{

			return true;
		}
		else
		{
			return false;
		}
	};

		
	
	//converts a position to the id of a given spot	
	int getID(position pos)
	{
		return pos.y * columnSize + pos.x;
	};

	//Finds out to which chain a position belongs to and returns the id of that chain.
	//If it doesn't belong to any chain it returns -1.
	int inWhichChain(position pos)
		{
			int id = getID(pos);

			for (int i = 0;i < stoneChainsSize;i++)
			{
				for (int j = 0;j < stoneChains[i].size;j++)
				{
					if (stoneChains[i].ids[j] == id)
					{
						return i;
					}
				}

			}
			return -1;
		};

		//gets all nearby positions and stores them in nearbyPos array
	void getNearby(position pos)
		{
			int counter = 0;
			for (int i = -1;i < 2;i += 2)
			{
				nearbyPos[counter] = position(pos.x + i, pos.y);
				nearbyPos[counter + 2] = position(pos.x, pos.y + i);
				counter++;
			}
		};

		//removes a chain of a given id from the array of stoneChains
	void removeFromStoneChains(int id)
		{
			stoneChain tmp1 = stoneChains[id];
			stoneChain tmp2 = stoneChains[stoneChainsSize-1];

			stoneChains[stoneChainsSize - 1] = tmp1;
			stoneChains[id] = tmp2;

			stoneChains[stoneChainsSize - 1].destroy();

			stoneChainsSize -= 1;
			stoneChains = (stoneChain*)realloc(stoneChains, stoneChainsSize * sizeof(stoneChain));
			
			
		};
		
		//makes stoneChains holding the right spots of connected stones
	void recalculateStoneChains()
	{
			for (int i = 0;i < stoneChainsSize;i++)
			{
				stoneChains[i].destroy();
			}
			free(stoneChains);

			stoneChains = NULL;
			stoneChainsSize = 0;

			positionVector alreadyChecked = positionVector();

			for (int x = 0; x < columnSize; x++)
			{
				for (int y = 0; y < rowSize; y++)
				{
					//if this position contains a stone or was searched previously
					if (tab[x].tab[y] != 0 && alreadyChecked.find(position(x, y)) == -1)
					{
						addNewStoneChain();
						stoneChains[stoneChainsSize - 1].color = tab[x].tab[y];

						positionVector openList = positionVector();

					

						char thisColor = tab[x].tab[y];

						openList.add(position(x, y));
						do
						{

							position pos = openList.tab[0];
							
							stoneChains[stoneChainsSize - 1].addStone(getID(pos));

							openList.shift();
							alreadyChecked.add(pos);

							getNearby(pos);

							for (int i = 0;i < 4;i++)
							{
								if (inBoard(nearbyPos[i]))
								{
									if (alreadyChecked.find(nearbyPos[i]) == -1 && openList.find(nearbyPos[i]) == -1)
									{
										if (tab[nearbyPos[i].x].tab[nearbyPos[i].y] == thisColor)
										{
											openList.add(nearbyPos[i]);
										}
									}

										
								}
							}

						} while (openList.size > 0);
						openList.destroy();

					
					}
				}
			}

			alreadyChecked.destroy();
			
			
		
	};
	//counts all empty neighbourghs and returns their number
	int countEmpty(position pos, positionVector * prevVector)
		{
			getNearby(pos);
			int result = 0;
			for (int i = 0;i < 4;i++)
			{
				if (inBoard(nearbyPos[i]))
				{
					if (tab[nearbyPos[i].x].tab[nearbyPos[i].y] == 0)
					{
					
						if ((*prevVector).find(nearbyPos[i]) == -1)
						{
							result++;
							(*prevVector).add(nearbyPos[i]);
						}


							
							
					
					}
				}
				
			}
			return result;
		}
	//converts a position to id
	position getPosition(int id)
		{

			return position(id % columnSize, id / columnSize);
		}

	//counts all liberties of a chain of a given id
	int countLiberties(int id)
	{
			int result = 0;

			positionVector prevVector = positionVector();

			for (int i = 0;i < stoneChains[id].size;i++)
			{
				int thisId = stoneChains[id].ids[i];
				result += countEmpty(getPosition(thisId), &prevVector);
			}

			prevVector.destroy();

			return result;
	}
	//checks if a chain has liberties
	bool hasLiberties(position pos, char color)
		{
			getNearby(pos);

			position nearbyAr[4] = { nearbyPos[0], nearbyPos[1], nearbyPos[2], nearbyPos[3] };

			for (int i = 0;i < 4;i++)
			{
				if (inBoard(nearbyAr[i]))
				{
					int which = inWhichChain(nearbyAr[i]);
					//if nearby position is in a chain and the color of that chain is the color of current player
					//check if that chain has more than one liberty
					if (which != -1 && stoneChains[which].color==playerStone)
					{
						if (countLiberties(which) > 1)
						{
							return true;
						}
					}
					//if this nearby spot is empty then there is at least one liberty
					else if (tab[nearbyAr[i].x].tab[nearbyAr[i].y] == 0)
					{
						return true;
					}
				}
				
			}
			return false;

		}
	//kills a chain of a given id, giving points to the current player and removing
	//all stones of this chain from the board
	void killThisChain(int id)
	{
			//removes stones that are killed and counts how many were killed
			int countKilled = 0;
			for (int i = 0;i < stoneChains[id].size;i++)
			{
				position pos = getPosition(stoneChains[id].ids[i]);
				tab[pos.x].tab[pos.y] = 0;
				countKilled++;
			}
			if (stoneChains[id].color == blackStone)
			{
				//for KO rule
				if (countKilled == 1)
				{
					lastTakenIDWhite = stoneChains[id].ids[0];
				}
				whitePoints += countKilled;
			}
			else 
			{
				//for KO rule
				if (countKilled == 1)
				{
					lastTakenIDBlack = stoneChains[id].ids[0];
				}
				blackPoints += countKilled;
			}
			removeFromStoneChains(id);
	};
		//kills all chains which lack liberties
		void killChains()
		{
			for (int i = 0;i < stoneChainsSize;i++)
			{
				if (countLiberties(i) == 0)
				{
					killThisChain(i);
				}
			}

		};
		//checks if a position that has no liberties can kill a nearby enemy chain
		//returns the id of chain to kill
		int fakeSuicide(position pos, char color)
		{
			getNearby(pos);
			position nearbyAr[4] = { nearbyPos[0], nearbyPos[1],nearbyPos[2],nearbyPos[3] };
			for (int i = 0;i < 4;i++)
			{
				if (inBoard(nearbyAr[i]))
				{
					int whichChain = inWhichChain(nearbyAr[i]);
					if (whichChain != -1)
					{
						int countLibs = countLiberties(whichChain);
						//if that chain is an enemy and has only this place as a liberty it can be killed.
						if (countLibs == 1 && stoneChains[whichChain].color!=color)
						{
							return whichChain;
						}
					}
				}
			}
				return -1;
		}
		bool isLegal(position pos, char color)
		{
			//ko rule
			position lastTakenPos = position(-1,-1);
			if (playerStone == whiteStone)
			{
				if (lastTakenIDWhite != -1)
				{
					lastTakenPos = getPosition(lastTakenIDWhite);
				}
				
			}
			else
			{
				if (lastTakenIDBlack != -1)
				{
					lastTakenPos = getPosition(lastTakenIDBlack);
				}
			
			}

			if (pos.x == lastTakenPos.x && pos.y == lastTakenPos.y)
			{
				return false;
			}

			if (playerStone == whiteStone)
			{
				lastTakenIDWhite = -1;
			}
			else
			{
				lastTakenIDBlack = -1;
			}

			if (isEmpty(pos))
			{

				if (hasLiberties(pos, color))
				{
					return true;
				}
				if (fakeSuicide(pos, color)!=-1)
				{
					killThisChain(fakeSuicide(pos, color));
					return true;
				}
				
			}
			return false;
		};
	
		bool placeStone(position pos)
		{
			//if a move is indeed legal, we put a stone, recalculate all stoneChains and then kill the dead ones.
			//Then we switch to the other player.
			if (isLegal(pos, playerStone))
			{
				
				tab[pos.x].tab[pos.y] = playerStone;
				recalculateStoneChains();
				killChains();

				if (setupMode)
				{
					//some kind of extra points for white player for playing an unfair game
					whitePoints += 0.5;
				}
				else
				{
					if (playerStone == whiteStone)
					{
						playerStone = blackStone;
					}
					else
					{
						playerStone = whiteStone;
					}
				}
					
					
					
					return true;
				
			}
			else
			{
				return false;
			}
		};
		
		bool isIgnored(int id)
		{
			
			for (int i = 0;i < chainsToIgnoreForNowSize;i++)
			{
				if (chainsToIgnoreForNow[i] == id)
				{
					return true;
				}
			}
			return false;
		}

	
		bool isWithoutFuture(stoneChain thisChain)
		{
			int countSpots = 0;
			for (int j = 0;j < thisChain.emptyGroupIdSize;j++)
			{
				

				int currentEmptyGroupId = thisChain.emptyGroupIds[j];
				emptyGroup currentGroup = emptyGroups[currentEmptyGroupId];

				if (currentGroup.touchesBlack && currentGroup.touchesWhite)
				{
					//ignore dame
				}
				else if ((currentGroup.touchesBlack && thisChain.color == blackStone) || (currentGroup.touchesWhite && thisChain.color == whiteStone))
				{
					countSpots++;
					if (currentGroup.size >= 2)
					{
						return false;
					}
				}

			}
			if (countSpots >= 2)
			{
				return false;
			}
			return true;
		}
		//finds chains to ignore for now
		void findChainsToIgnore()
		{
			free(chainsToIgnoreForNow);

			chainsToIgnoreForNow = NULL;
			chainsToIgnoreForNowSize = 0;

			for (int i = 0;i < stoneChainsSize;i++)
			{
				if (isWithoutFuture(stoneChains[i]))
				{
					chainsToIgnoreForNowSize++;
					chainsToIgnoreForNow = (int*)realloc(chainsToIgnoreForNow, chainsToIgnoreForNowSize*sizeof(int));
					
					chainsToIgnoreForNow[chainsToIgnoreForNowSize - 1] = i;
				}
			}
		}
		//finds the color of the empty group on this position (used solely for game end)
		char findEmptyGroupColor(position pos)
		{
			for (int i = 0;i < emptyGroupSize;i++)
			{
				if (emptyGroups[i].find(pos))
				{
					if (emptyGroups[i].touchesBlack && emptyGroups[i].touchesWhite)
					{
						return 0;
					}
					else if (emptyGroups[i].touchesBlack)
					{
						return blackStone;
					}
					else if (emptyGroups[i].touchesWhite)
					{
						return whiteStone;
					}
				}
			
			}
			return 0;
		}
		//removes chains without a future when the game ends
		void removeDeadAtGameEnd()
		{
			//recalculates all empty groups
			recalculateEmptyGroups();

			bool removedChain = false;
			
			for (int i = 0;i < stoneChainsSize;i++)
			{
				//checks if the empty groups belonging to this chain have more than two separate emptyGroups or if the size of one of them is
				//>=2. If yes, we consider them unsettled. Else they are considered dead.
				
				//if a chain is without future we kill it
				if (!gameEnded)
				{
					if (isWithoutFuture(stoneChains[i]) && !isIgnored(i))
					{
						removedChain = true;
						killThisChain(i);
						i = stoneChainsSize + 2;
					}
				}
				else
				{
					if (isIgnored(i))
					{
						int anySpotId = stoneChains[i].ids[0];
						position firstPos = getPosition(anySpotId);

						if (findEmptyGroupColor(firstPos) != 0 && findEmptyGroupColor(firstPos) != stoneChains[i].color)
						{
							removedChain = true;
							killThisChain(i);
							i = stoneChainsSize + 2;
						}
					}
				}
				
			}
			
			//if we removed a chain we need to recalculate all chains again, to include all newly made emptyGroups.
			if (removedChain)
			{
				removeDeadAtGameEnd();
			}
			else
			{
				if (gameEnded)
				{
					countTerritories();
				}
				
			}

		}

		//counts the scores from territories belonging to each player
		void countTerritories()
		{
			

			for (int i = 0; i < emptyGroupSize;i++)
			{
				emptyGroup thisOne = emptyGroups[i];

				if (thisOne.touchesBlack && thisOne.touchesWhite)
				{
					///do nothing
				}
				else if (thisOne.touchesBlack)
				{
					blackPoints += thisOne.size;
				}
				else if (thisOne.touchesWhite)
				{
					whitePoints += thisOne.size;
				}

				thisOne.destroy();
			}
		}

		//recalculates all emptyGroups to include all connected empty spots.
		void recalculateEmptyGroups()
		{
			positionVector alreadyChecked = positionVector();
			
			for (int i = 0;i < emptyGroupSize;i++)
			{
				emptyGroups[i].destroy();
			}
			free(emptyGroups);

			emptyGroups = NULL;
			emptyGroupSize = 0;

			for (int i = 0;i < stoneChainsSize;i++)
			{
				stoneChains[i].resetEmptyGroupIds();
			}

			for (int x = 0; x < columnSize; x++)
			{
				for (int y = 0; y < rowSize; y++)
				{
					//if a spot is  empty and not previously checked
					if (tab[x].tab[y] == 0 && alreadyChecked.find(position(x,y))==-1)
					{
						//we make a new emptyGroup with this position
						emptyGroup thisGroup = emptyGroup();
						thisGroup.add(position(x, y));
						
						//positionVector of positions we still need to check
						positionVector toCheck = positionVector();

						
						getNearby(position(x,y));

						//adds all nearby positions to vector holding the ones we need to check
						for (int i = 0;i < 4;i++)
						{
							if (inBoard(nearbyPos[i]))
							{
								toCheck.add(nearbyPos[i]);
							}
							
						}
						//while we have any position left to check
						while (toCheck.size > 0)
						{
						
							//get the position we need to check and remove it from our list
							position currentPos = toCheck.tab[0];
							toCheck.remove(currentPos);

							//if it wasn't previously checked at all
							if (alreadyChecked.find(currentPos) == -1)
							{
								
								int whichChain = inWhichChain(currentPos);
								
								if (isIgnored(whichChain))
								{
									alreadyChecked.add(currentPos);

									thisGroup.add(currentPos);

									getNearby(currentPos);

									for (int i = 0;i < 4;i++)
									{
										if (inBoard(nearbyPos[i]))
										{
											toCheck.add(nearbyPos[i]);
										}

									}

								}
								else
								{
									switch (tab[currentPos.x].tab[currentPos.y])
									{
									case 0:
										//if empty we add the neighbours of this one to our list
										alreadyChecked.add(currentPos);

										thisGroup.add(currentPos);

										getNearby(currentPos);

										for (int i = 0;i < 4;i++)
										{
											if (inBoard(nearbyPos[i]))
											{
												toCheck.add(nearbyPos[i]);
											}

										}

										break;
									case whiteStone:
										//if this is a white stone we inform the stoneChain that it touches this group
										//and that this group touches white stones

										thisGroup.touchesWhite = 1;

										if (whichChain != -1)
										{
											stoneChains[whichChain].addEmptyGroupId(emptyGroupSize);
										}

										break;
									case blackStone:
										//if this is a blackStone stone we inform the stoneChain that it touches this group
										//and that this group touches black stones
										thisGroup.touchesBlack = 1;

										if (whichChain != -1)
										{
											stoneChains[whichChain].addEmptyGroupId(emptyGroupSize);
										}


										break;
									}
								}
								
							}
							
						}
						toCheck.destroy();




						emptyGroupSize++;
						emptyGroups = (emptyGroup*)realloc(emptyGroups, sizeof(emptyGroup)*emptyGroupSize);

						emptyGroups[emptyGroupSize - 1] = thisGroup;
					}
				}
			}
			
			
			alreadyChecked.destroy();

			
			
			
		};
		void destroy()
		{
			for (int i = 0;i < columnSize;i++)
			{
				tab[i].destroy();
			}

			for (int i = 0;i < stoneChainsSize;i++)
			{
				stoneChains[i].destroy();
			}
			if (!gameEnded)
			{
				for (int i = 0;i < emptyGroupSize;i++)
				{
					emptyGroups[i].destroy();
				}
			}
			

			free(tab);
			free(stoneChains);
			free(emptyGroups);

			free(chainsToIgnoreForNow);
		}
};


//a class that holds a simplified string
class myString
{
	public:

		char* tab = NULL;
		int currentSize = 0;

		//adds a char to the char array
		void addChar(char chr)
		{
				currentSize += 1;
				tab = (char*)realloc(tab, sizeof(char) * currentSize);
				if (tab[currentSize - 2] == '\0')
				{
					tab[currentSize - 2] = chr;
					tab[currentSize - 1] = '\0';
				}
				else
				{
					
					tab[currentSize - 1] = chr;
				}
			
				

				
				tab[currentSize - 1] = chr;
			
			
			
		};
		//adds an array of chars to the char array
		void addCharAr(char * chr)
		{
			
			int Arsize = 0;
			while (*(chr + Arsize) != '\0')
			{
				addChar(*(chr + Arsize));
				Arsize++;
			}
			

			
			
		};
		
		//reallocates the space allocated in such a way that it no longer contains the last char
		//basically removing it
		void pop()
		{
			currentSize -= 1;
			tab = (char*)realloc(tab, sizeof(char) * currentSize);
		};
		
		//converts a char to an int and returns it
		int toInt()
		{
			int result = 0;
			int tenMultiplier = 1;
			for (int i = currentSize - 1;i >= 0;i--)
			{
				if (tab[i] < '0' || tab[i]>'9')
				{
					return -1;
				}
				result += (tab[i] - '0') * tenMultiplier;
				tenMultiplier *= 10;
			}
			return result;
		};

		//constructor without arguments
		myString()
		{
			
			
			int currentSize = 0;

			tab = NULL;
		};

		//constructor with initial char array
		myString(char* initial)
		{
			int it = 0;
			while (*(initial + it) != '\0')
			{
				it++;
			}
			it++;

			int initialSize = it * sizeof(char);
			currentSize = initialSize;

			tab = (char*)calloc(initialSize, sizeof(char));

			for (int i = 0; i < initialSize;i++)
			{

				tab[i] = *(initial + i);
			}
		};
		
		void destroy()
		{
			free(tab);
		}
};

//sets both the textcolor and textbackground to one of the specified styles
void setStyles(int type)
{
	switch (type)
	{
		case gameBoardSpacesStyle:
			textcolor(gameBoardSpacesColor);
			textbackground(gameBoardSpacesBackgroundColor);
			break;
		case borderStyle:
			textcolor(borderColor);
			textbackground(borderBackgroundColor);
			break;
		case defaultStyle:
			textcolor(WHITE);
			textbackground(BLACK);
			break;
		default:
			break;
	}
}

//marks all stoneChains with only one liberty with a specified background color
void showAtariWarnings(board gameBoard)
{
	
	textbackground(atariWarning);

	for (int i = 0;i < gameBoard.stoneChainsSize;i++)
	{
		if (gameBoard.countLiberties(i) == 1)
		{
			if (gameBoard.stoneChains[i].color == whiteStone)
			{
				textcolor(WHITE);
			}
			else
			{
				textcolor(BLACK);
			}

			for (int c = 0;c < gameBoard.stoneChains[i].size;c++)
			{
				int currentID = gameBoard.stoneChains[i].ids[c];
				position pos = gameBoard.getPosition(currentID);

				gotoxy(boardOffsetX + pos.x, boardOffsetY + pos.y);
				putch('O');
			}
		}
	}
	setStyles(defaultStyle);
}

//prints the content of our board contained in a chunk and its border
void printBoard(board gameBoard, text_info txtInfo)
{
	setStyles(gameBoardSpacesStyle);
	
	//prints the content of the current chunk
	for (int x=0; x< gameBoard.chunkSizeX&&x<gameBoard.columnSize; x++)
	{
		for (int y = 0;y < gameBoard.chunkSizeY&&y<gameBoard.rowSize; y++)
		{
			gotoxy(x+boardOffsetX, y+boardOffsetY);
			if (gameBoard.tab[x+gameBoard.cursorOffset.x].tab[y+gameBoard.cursorOffset.y] == whiteStone)
			{
				textcolor(WHITE);
				putch('O');
				
			}
			else if (gameBoard.tab[x + gameBoard.cursorOffset.x].tab[y + gameBoard.cursorOffset.y] == blackStone)
			{
				textcolor(BLACK);
				putch('O');
			}
			else
			{
				setStyles(gameBoardSpacesStyle);
				putch('+');
			}
			
		}
		
	}

	//from here on out it prints borders only if they should be visible
	setStyles(borderStyle);
	//up border
	if (gameBoard.cursorOffset.y == 0)
	{
		for (int x = -1;x <= gameBoard.chunkSizeX;x++)
		{
			gotoxy(x + boardOffsetX, boardOffsetY - 1);
			putch(borderSymbol);
		}
	}
	//down border
	if (gameBoard.cursorOffset.y+gameBoard.chunkSizeY == gameBoard.rowSize)
	{
		for (int x = -1;x <= gameBoard.chunkSizeX;x++)
		{
			gotoxy(x + boardOffsetX, boardOffsetY + gameBoard.chunkSizeY);
			putch(borderSymbol);
		}
	}
	//left border
	if (gameBoard.cursorOffset.x == 0)
	{
		for (int y = 0;y < gameBoard.chunkSizeY;y++)
		{
			gotoxy(boardOffsetX - 1, boardOffsetY + y);
			putch(borderSymbol);
		}
	}
	
	//right border
	if (gameBoard.cursorOffset.x + gameBoard.chunkSizeX == gameBoard.columnSize)
	{
		for (int y = 0;y < gameBoard.chunkSizeY;y++)
		{
			gotoxy(boardOffsetX + gameBoard.chunkSizeX, boardOffsetY + y);
			putch(borderSymbol);
		}
	}
	
	setStyles(defaultStyle);
}

//checks if both x and y fit inside our chunk
bool isInsideChunk(int x, int y, board gameBoard, text_info txtInfo)
{
	int realX = x - boardOffsetX;
	int realY = y - boardOffsetY;

	
	if (realX >= gameBoard.chunkSizeX || realX < 0)
	{
		return false;
	}

	if (realY >= gameBoard.chunkSizeY || realY < 0)
	{
		return false;
	}
	return true;
}

//checks if both x and y would fit on screen if we moved our chunk offset
bool wouldBeInsideBoard(int x, int y, board gameBoard, text_info txtInfo)
{
	int realBoardX = x - boardOffsetX+gameBoard.cursorOffset.x;
	int realBoardY = y - boardOffsetY + gameBoard.cursorOffset.y;

	if (realBoardX >= gameBoard.columnSize || realBoardX < 0)
	{
		return false;
	}
	if (realBoardY >= gameBoard.rowSize || realBoardY < 0)
	{
		return false;
	}
	return true;
}

//sets the value of the variable pointed by str pointer to a myString variable containing obtained
//char array
void inputObtainer(myString* str, myString prompt)
{
	char currentKey = ' ';
	int displayedCharacters = 0;

	gotoxy(wherex() + 1, wherey());

	//prints the prompt
	for (int i = 0;i < prompt.currentSize;i++)
	{
		putch(prompt.tab[i]);
		
	}
	//while the key pressed is not enter
	while (currentKey != ENTER)
	{
		currentKey = getch();

		//removes the last character seen
		if (currentKey == BACKSPACE)
		{
			if (displayedCharacters != 0)
			{
				//A hack used to visually erase the last character.
				//Putting a backspace will only erase the last character when another is pressed, 
				// which isn't user-friendly or helpful at all.
				//That's why we put a nullcharacter and then use backspace again.
				//This way we replace a nullcharacter with our new character
				//
				putch(BACKSPACE);
				putch(NULLCHR);
				putch(BACKSPACE);
				displayedCharacters--;
				(*str).pop();
			}
		}
		
		else if (currentKey != ENTER)
		{
			(*str).addChar(currentKey);
			putch(currentKey);
			displayedCharacters++;
		}

	}
	prompt.destroy();
}

//function that is responsible for making a new board and returning it.
board makeNewBoard(int * legendY)
{
	//displays a menu to tell the user what he can press
	printInLegend("1) 9x9", legendY);
	printInLegend("2) 13x13", legendY);
	printInLegend("3) 19x19", legendY);
	printInLegend("4) custom (>1x1)", legendY);

	char result = getch();

	int xSize = 0;
	int ySize = 0;
	

	myString xString = myString();
	myString yString = myString();

	char message1[] = "X size: ";
	char message2[] = "Y size: ";

	//makes a default 9x9 board in case something goes wrong
	board resultBoard = board(9, 9);

	switch (result)
	{
	case '2':
		resultBoard.destroy();
		resultBoard = board(13, 13);
		break;
	case '3':
		resultBoard.destroy();
		resultBoard = board(19, 19);
		break;
	case '4':
		//asks for the xSize of the board and obtains it
		gotoxy(wherex() + 1, wherey());
		inputObtainer(&xString, myString(message1));

		//asks for the ySize of the board and obtains it
		gotoxy(wherex() + 1, wherey());
		inputObtainer(&yString, myString(message2));
	
		//converts the sizes to integers
		xSize = xString.toInt();
		ySize = yString.toInt();


		if (xSize > 0 && ySize > 0)
		{
			resultBoard.destroy();
			resultBoard = board(xSize, ySize);
		}
		break;
	default:
		break;
	}


	gotoxy(legendOffsetX, wherey()+1);
	
	myString response = myString();
	char question[] = "Do you want to setup black? [y/n]: ";

	inputObtainer(&response, question);
	
	
	if (response.tab[0] == 'y')
	{
		resultBoard.playerStone = blackStone;
		resultBoard.setupMode = true;
	}

	xString.destroy();
	yString.destroy();
	response.destroy();

	return resultBoard;
	
}

//function for debugging stoneChains, normally not used
void stoneChainsDebug(board gameBoard)
{
	for (int i = 0;i < gameBoard.stoneChainsSize;i++)
	{
		for (int j = 0;j < gameBoard.stoneChains[i].size;j++)
		{
			position pos = gameBoard.getPosition(gameBoard.stoneChains[i].ids[j]);
				
			gotoxy(boardOffsetX+pos.x, boardOffsetY+pos.y);
			if (gameBoard.stoneChains[i].color == whiteStone)
			{
				textbackground(WHITE);
				textcolor(BLACK);
			}
			else
			{
				textbackground(BLACK);
				textcolor(WHITE);
			}

			//putch(gameBoard.countLiberties(i)+48);
			putch(gameBoard.inWhichChain(pos) + 48);

		}
	}
}

//function that saves the entire board to a file
void saveGame(board * gameBoard, int * legendY)
{
	//getting the save name from the user
	myString saveName = myString();
	char message[] = "save file name: ";

	gotoxy(legendOffsetX, legendOffsetY+*legendY+1);
	inputObtainer(&saveName, message);

	//adding txt extension to the file name
	char extension[] = ".txt";
	saveName.addCharAr(extension);
	saveName.addChar('\0');
	FILE* plik = fopen(saveName.tab, "w+");

	if (plik != NULL)
	{
		//writing neccesary data to a file

		fwrite(&(*gameBoard).columnSize, sizeof(int), 1, plik);
		fwrite(&(*gameBoard).rowSize, sizeof(int), 1, plik);
		fwrite(&(*gameBoard).playerStone, sizeof(int), 1, plik);
		fwrite(&(*gameBoard).whitePoints, sizeof(float), 1, plik);
		fwrite(&(*gameBoard).blackPoints, sizeof(float), 1, plik);
		fwrite(&(*gameBoard).setupMode, sizeof(bool), 1, plik);
		fwrite(&(*gameBoard).lastTakenIDWhite, sizeof(int), 1, plik);
		fwrite(&(*gameBoard).lastTakenIDBlack, sizeof(int), 1, plik);

		for (int i = 0;i < (*gameBoard).columnSize;i++)
		{
			for (int j = 0; j < (*gameBoard).rowSize;j++)
			{
				fwrite(&(*gameBoard).tab[i].tab[j], sizeof(char), 1, plik);
			}
			
			
		}
		fclose(plik);
	}
	
	saveName.destroy();

}

//loads game from a file
board loadGame(int * legendY, board * gameBoard)
{
	//obtaining the save name
	char message[] = "save file name: ";
	myString saveName = myString();

	gotoxy(legendOffsetX, legendOffsetY + *legendY + 1);
	inputObtainer(&saveName, message);

	//adding the extension
	char extension[] = ".txt";
	saveName.addCharAr(extension);
	saveName.addChar('\0');
	FILE* plik = fopen(saveName.tab, "r");

	saveName.destroy();

	board newBoard = board(9, 9);

	if (plik != NULL)
	{
		(*gameBoard).destroy();

		//reading all data from file
		int boardSizeX = 0;
		int boardSizeY = 0;
		fread(&boardSizeX, sizeof(int), 1, plik);
		fread(&boardSizeY, sizeof(int), 1, plik);
		newBoard = board(boardSizeX, boardSizeY);
		fread(&(newBoard.playerStone), sizeof(int), 1, plik);
		fread(&(newBoard.whitePoints), sizeof(float), 1, plik);
		fread(&(newBoard.blackPoints), sizeof(float), 1, plik);
		fread(&(newBoard.setupMode), sizeof(bool), 1, plik);
		fread(&(newBoard.lastTakenIDWhite), sizeof(int), 1, plik);
		fread(&(newBoard.lastTakenIDBlack), sizeof(int), 1, plik);

		for (int i = 0;i < newBoard.columnSize;i++)
		{
			for (int j = 0;j < newBoard.rowSize;j++)
			{
				fread(&newBoard.tab[i].tab[j], sizeof(char), 1, plik);
			
			}
			
		}
		fclose(plik);
	
		newBoard.recalculateStoneChains();
		newBoard.recalculateEmptyGroups();

		return newBoard;
	}
	else
	{
		//if there is an error it returns the same board
		newBoard.destroy();

		return (*gameBoard);
	}
	
}

//sets proper chunk sizes
void recalculateChunks(board* gameBoard, text_info txtInfo)
{
	if (boardOffsetX > legendOffsetX)
	{
		(*gameBoard).chunkSizeX = txtInfo.screenwidth - boardOffsetX - 2;
		(*gameBoard).chunkSizeY = txtInfo.screenheight - boardOffsetY - 2;
	}
	else
	{
		(*gameBoard).chunkSizeX = legendOffsetX-boardOffsetX - 2;
		(*gameBoard).chunkSizeY = txtInfo.screenheight - boardOffsetY - 2;
	}
	
	if ((*gameBoard).chunkSizeX < 0 || (*gameBoard).chunkSizeY < 0)
	{
		(*gameBoard).chunkSizeX = 0;
		(*gameBoard).chunkSizeY = 0;
	}

	if ((*gameBoard).chunkSizeX > (*gameBoard).columnSize)
	{
		(*gameBoard).chunkSizeX = (*gameBoard).columnSize;
	}

	if ((*gameBoard).chunkSizeY > (*gameBoard).rowSize)
	{
		(*gameBoard).chunkSizeY = (*gameBoard).rowSize;
	}
}

//returns a copy of the board pointed to by gameBoard pointer
board cloneBoard(board* gameBoard)
{
	board copyBoard = board((*gameBoard).columnSize, (*gameBoard).rowSize);
	copyBoard.playerStone = (*gameBoard).playerStone;
	copyBoard.whitePoints = (*gameBoard).whitePoints;
	copyBoard.blackPoints = (*gameBoard).blackPoints;
	copyBoard.lastTakenIDWhite = (*gameBoard).lastTakenIDWhite;
	copyBoard.lastTakenIDWhite = (*gameBoard).lastTakenIDBlack;

	for (int i = 0;i < copyBoard.columnSize;i++)
	{
		for (int j = 0;j < copyBoard.rowSize;j++)
		{
			copyBoard.tab[i].tab[j] = (*gameBoard).tab[i].tab[j];
		}
	}
	copyBoard.recalculateStoneChains();
	copyBoard.recalculateEmptyGroups();
	return copyBoard;
}
//function that checks if the game can continue without at least one of the players losing points

void endGame(board* gameBoard, text_info txtInfo)
{
	
	(*gameBoard).findChainsToIgnore();
	(*gameBoard).removeDeadAtGameEnd();

	(*gameBoard).gameEnded = true;
	(*gameBoard).removeDeadAtGameEnd();
	
}

bool checkIfMovesAvailable(board* gameBoard, text_info txtInfo)
{
	board gameBoardIfEndsNow = cloneBoard(gameBoard);
	endGame(&gameBoardIfEndsNow, txtInfo);

	//check if black can continue
	for (int x = 0;x < gameBoardIfEndsNow.columnSize; x++)
	{
		for (int y = 0; y < gameBoardIfEndsNow.rowSize; y++)
		{
			board thisTestBoard = cloneBoard(gameBoard);

			thisTestBoard.playerStone = blackStone;

			if (thisTestBoard.placeStone(position(x, y)))
			{
				endGame(&thisTestBoard, txtInfo);

				if (thisTestBoard.blackPoints >= gameBoardIfEndsNow.blackPoints)
				{
					return true;
				}
				
				
			}
			thisTestBoard.destroy();
		}
	}

	//check if white can continue
	for (int x = 0;x < gameBoardIfEndsNow.columnSize; x++)
	{
		for (int y = 0; y < gameBoardIfEndsNow.rowSize; y++)
		{
			board thisTestBoard = cloneBoard(gameBoard);
			thisTestBoard.playerStone = whiteStone;

			if (thisTestBoard.placeStone(position(x, y)))
			{
				endGame(&thisTestBoard, txtInfo);

				if (thisTestBoard.whitePoints >= gameBoardIfEndsNow.whitePoints)
				{
					return true;
				}
			
			}

			thisTestBoard.destroy();
		}
	}
	
	gameBoardIfEndsNow.destroy();
	
	return false;
}


void printWholeLegend(int * legendY, board gameBoard, int x, int y)
{
	char txt[32];

	printInLegend("Michal Grabowski 193314", legendY);
	printInLegend("a b c d e f g h i j k l m n", legendY);
	printInLegend("q       = exit", legendY);
	printInLegend("cursors = moving", legendY);
	printInLegend("space   = change color", legendY);
	printInLegend("enter   = accept action and end turn", legendY);
	printInLegend("esc   = cancel action", legendY);
	printInLegend("n	= new game", legendY);
	printInLegend("i	= place stone", legendY);
	printInLegend("s	= save game", legendY);
	printInLegend("l	= load game", legendY);

	if (!gameBoard.gameEnded)
	{
		if (gameBoard.setupMode)
		{
			printInLegend("x	= disable setup mode", legendY);
		}
		printInLegend("\n", legendY);
		printInLegend("Player turn:", legendY);
		if (gameBoard.playerStone == whiteStone)
		{
			textcolor(BLACK);
			textbackground(WHITE);
			puts(" _WHITE_ ");
		}
		else
		{
			textcolor(WHITE);
			textbackground(BLACK);
			puts(" _BLACK_ ");
		}
		setStyles(defaultStyle);
	}
	else
	{
		printInLegend(" ", legendY);
		printInLegend("  GAME OVER", legendY);
		printInLegend(" ", legendY);
	}
	printInLegend("WHITE \t BLACK", legendY);

	sprintf_s(txt, "%.1f \t %.1f", gameBoard.whitePoints, gameBoard.blackPoints);
	printInLegend(txt, legendY);

	sprintf_s(txt, "Position on screen: (%d, %d)", x - boardOffsetX, y - boardOffsetY);
	printInLegend(txt, legendY);

	sprintf_s(txt, "Real position: (%d, %d)", x - boardOffsetX + gameBoard.cursorOffset.x, y - boardOffsetY + gameBoard.cursorOffset.y);
	printInLegend(txt, legendY);

	sprintf_s(txt, "Chunk size: (%d, %d)", gameBoard.chunkSizeX, gameBoard.chunkSizeY);
	printInLegend(txt, legendY);
}


int main() {
	int zn = 0, x = 40, y = 12, attr = 7, back = 0, zero = 0;
	

#ifndef __cplusplus
	Conio2_Init();
#endif
	// settitle sets the window title
	settitle("Michal, Grabowski, 193314");
	board gameBoard = board(9, 9);
	x = boardOffsetX + gameBoard.columnSize / 2;
	y = boardOffsetY + gameBoard.rowSize / 2;
	// hide the blinking cursor
	_setcursortype(_NOCURSOR);

	int previousChunkSizeX = 0;
	int previousChunkSizeY = 0;
	do {
		textbackground(BLACK);
		clrscr();
		textcolor(7);
		//this gets the info about the window we need
		text_info txtInfo;
		gettextinfo(&txtInfo);
		//											  ^
		//recalculates chunks using the info obtained |
		//											  |
		recalculateChunks(&gameBoard, txtInfo);
		int legendY = 0;

		printWholeLegend(&legendY, gameBoard, x, y);


		printBoard(gameBoard, txtInfo);
		showAtariWarnings(gameBoard);
		//stoneChainsDebug(gameBoard);

		//if the chunk size changed we center the cursor
		if (gameBoard.chunkSizeX != previousChunkSizeX || gameBoard.chunkSizeY != previousChunkSizeY)
		{
			x = boardOffsetX + gameBoard.chunkSizeX / 2;
			y = boardOffsetY + gameBoard.chunkSizeY / 2;
		}

		gotoxy(x, y);
		textcolor(attr);
		textbackground(cursorBackgroundColor);
		putch('*');

		textbackground(back);
		
		zero = 0;
		zn = getch();
		
		if (zn == 0) {
			zero = 1;		
			zn = getch();
			recalculateChunks(&gameBoard, txtInfo);
			if (zn == UParrow)
			{
				if (isInsideChunk(x, y - 1, gameBoard, txtInfo))
				{
					y--;
				}
				else if (wouldBeInsideBoard(x, y - 1, gameBoard, txtInfo))
				{
					gameBoard.cursorOffset.y -= 1;

				}
			}
			else if (zn == DOWNarrow)
			{
				if (isInsideChunk(x, y + 1, gameBoard, txtInfo))
				{
					y++;
				}
				else if (wouldBeInsideBoard(x, y + 1, gameBoard, txtInfo))
				{
					gameBoard.cursorOffset.y += 1;

				}
				
			}
			else if (zn == LEFTarrow)
			{
				if (isInsideChunk(x - 1, y, gameBoard, txtInfo))
				{
					x--;
				}
				else if (wouldBeInsideBoard(x - 1, y, gameBoard, txtInfo))
				{
					gameBoard.cursorOffset.x -= 1;

				}
			
			}
			else if (zn == RIGHTarrow)
			{
				if (isInsideChunk(x + 1, y, gameBoard, txtInfo))
				{
					x++;
				}
				else if (wouldBeInsideBoard(x + 1, y, gameBoard, txtInfo))
				{
					gameBoard.cursorOffset.x += 1;

				}
			}
			
		}
		else if (zn == 'n')
		{
	
			gameBoard.destroy();
	
			gameBoard = makeNewBoard(&legendY);

			recalculateChunks(&gameBoard, txtInfo);
			x = boardOffsetX + gameBoard.chunkSizeX / 2;
			y = boardOffsetY + gameBoard.chunkSizeY / 2;
				
					
		}
		else if (zn == 'i')
		{
			if (!gameBoard.gameEnded)
			{
				if (gameBoard.isLegal(position(x - boardOffsetX + gameBoard.cursorOffset.x, y - boardOffsetY + gameBoard.cursorOffset.y), gameBoard.playerStone))
				{
					printInLegend("\n Press enter to confirm or esc to cancel", &legendY);
					while (zn != ESC && zn != ENTER)
					{
						zn = getch();
					}
					if (zn == ENTER)
					{
						gameBoard.placeStone(position(x - boardOffsetX + gameBoard.cursorOffset.x, y - boardOffsetY + gameBoard.cursorOffset.y));
						
						if (!checkIfMovesAvailable(&gameBoard, txtInfo))
						{
							endGame(&gameBoard, txtInfo);
							printBoard(gameBoard, txtInfo);
						}
					}
					

				}
				
			
			}
			
		}
		else if (zn == 'l')
		{
			gameBoard = loadGame(&legendY, &gameBoard);
			recalculateChunks(&gameBoard, txtInfo);

			x = boardOffsetX + gameBoard.chunkSizeX / 2;
			y = boardOffsetY + gameBoard.chunkSizeY / 2;

			gotoxy(x, y);
			
		}
		else if (zn == 's'&& !gameBoard.gameEnded)
		{
			
			saveGame(&gameBoard, &legendY);
			
			
			
		}
		else if (zn == 'x' && !gameBoard.gameEnded)
		{
			gameBoard.playerStone = whiteStone;
			gameBoard.setupMode = false;

		}
		else if (zn == 'f' && !gameBoard.gameEnded)
		{
			endGame(&gameBoard, txtInfo);
			printBoard(gameBoard, txtInfo);

		}
		previousChunkSizeX = gameBoard.chunkSizeX;
		previousChunkSizeY = gameBoard.chunkSizeY;
	} while (zn != 'q');


	gameBoard.destroy();

	_setcursortype(_NORMALCURSOR);

	// show the cursor so it will be
	// visible after the program ends
	return 0;
}
