#include "CJumpPointSearch.h"
#include "protocol.h"
#include <math.h>

extern MAP g_Map[WORLD_HEIGHT][WORLD_WIDTH];

enum DIR_TYPE
{
	eDIR_UL, eDIR_UU, eDIR_UR,		//012
	eDIR_LL, eDIR_RR = 5,	//3 5
	eDIR_DL, eDIR_DD, eDIR_DR		//678
};


CJumpPointSearch::CJumpPointSearch()
{
}

CJumpPointSearch::~CJumpPointSearch()
{
}

void CJumpPointSearch::setEndPos(int x, int y)
{
	m_iEndX = x;
	m_iEndY = y;
}

void CJumpPointSearch::setStartPos(int x, int y)
{
	m_iStartX = x;
	m_iStartY = y;
}

bool CJumpPointSearch::checkWalkAble(int _x, int _y)
{
	if(g_Map[_y][_x].type == eBLOCKED)
		return false;

	return true;
}

void CJumpPointSearch::setG(NODE* _node)
{
	if (_node->pParent != nullptr)
		_node->G = _node->pParent->G + 1;
	else
		_node->G = 0;
}

void CJumpPointSearch::setG_dia(NODE * _node)
{
	if (_node->pParent != nullptr)
		_node->G = _node->pParent->G + 1.5;
	else
		_node->G = 0;
}

void CJumpPointSearch::setH(NODE * _node)
{
	_node->H = abs(_node->ix - m_iEndX) + abs(_node->iy - m_iEndY);
}

void CJumpPointSearch::setF(NODE * _node)
{
	_node->F = _node->G + _node->H;
}

void CJumpPointSearch::checkDirection(int** mapData, NODE* _pNode, int _x, int _y, int _dir)
{
	int jumpX;
	int jumpY;

	//점프해서 노드를 만들 위치 jump에 담아오기
	if(findJumpNode(_x, _y, _dir, jumpX, jumpY))
	{
		//이미 open되어있다면 g값 비교
		if (mapData[jumpY][jumpX] == eOPEN)
		{
			std::list<NODE*>::iterator iter;

			for (iter = openList.begin(); iter != openList.end(); ++iter)
			{
				if ((*iter)->iy == jumpY && (*iter)->ix == jumpX)
				{
					if (_dir % 2 == 0) // 대각선
					{
						if ((*iter)->G >= _pNode->G + 1.5)
						{
							(*iter)->pParent = _pNode;
							(*iter)->G = _pNode->G + 1.5;
							(*iter)->F = (*iter)->G + (*iter)->H;
							mapData[(*iter)->iy][(*iter)->ix] = eOPEN;
						}
					}
					else //직선
					{
						if ((*iter)->G >= _pNode->G + 1.0)
						{
							(*iter)->pParent = _pNode;
							(*iter)->G = _pNode->G + 1.0;
							(*iter)->F = (*iter)->G + (*iter)->H;
							mapData[(*iter)->iy][(*iter)->ix] = eOPEN;
						}
					}
					break;
				}
			}
		}
		//없다면 새로 노드 만들기
		else if (g_Map[jumpY][jumpX].type == eBLANK || g_Map[jumpY][jumpX].type == eEND)
		{
			NODE* newNode = new NODE(jumpX, jumpY, _pNode);
			if (_dir % 2 == 0)	setG_dia(newNode);
			else				setG(newNode);
			setH(newNode);
			setF(newNode);
			openList.push_back(newNode);
			mapData[jumpY][jumpX] = eOPEN;
		}
	}
	

}

bool CJumpPointSearch::findJumpNode(int _x, int _y, int _dir, int& _jumpX, int& _jumpY)
{
	if (_x < 0 || _x >= WORLD_WIDTH) return false;
	if (_y < 0 || _y >= WORLD_HEIGHT) return false;
	if (g_Map[_y][_x].type==eBLOCKED) return false;

	_jumpX = _x;
	_jumpY = _y;

	if (_x == m_iEndX && _y == m_iEndY) 
		return true;

	//코너나, 저 멀리 코너를 찾게되면 좌표를 jump에 담아서 return true;
	switch (_dir)
	{
		//↖
	case eDIR_UL:
		if (_y + 1 < WORLD_HEIGHT && _x - 1 >= 0)
		{
			if (g_Map[_y + 1][_x].type == eBLOCKED && g_Map[_y + 1][_x - 1].type == eBLANK)
				return true;
		}

		if (_x + 1 < WORLD_WIDTH && _y - 1 >= 0)
		{
			if(g_Map[_y][_x + 1].type == eBLOCKED && g_Map[_y - 1][_x + 1].type == eBLANK)
				return true;
		}

		if (findJumpNode(_x, _y - 1, eDIR_UU, _jumpX, _jumpY))
		{
			_jumpX = _x;
			_jumpY = _y;
			return true;
		}

		if (findJumpNode(_x - 1, _y, eDIR_LL, _jumpX, _jumpY))
		{
			_jumpX = _x;
			_jumpY = _y;
			return true;
		}

		findJumpNode(_x - 1, _y - 1, _dir, _jumpX, _jumpY);
		break;

		//↑
	case eDIR_UU:
		//o
		//x ㅁ
		if (_x - 1 >= 0 && _y - 1 >= 0)
		{
			if ((g_Map[_y][_x - 1].type == eBLOCKED && g_Map[_y - 1][_x - 1].type == eBLANK))
				return true;
		}
		//   o
		//ㅁ x 
		if (_x + 1 < WORLD_WIDTH && _y - 1 >= 0)
		{
			if (g_Map[_y][_x + 1].type == eBLOCKED && g_Map[_y - 1][_x + 1].type == eBLANK)
				return true;
		}

		//진행방향 탐색
		findJumpNode(_x, _y - 1, _dir, _jumpX, _jumpY);
		break;

		//↗
	case eDIR_UR:

		//O
		//Xㅁ
		if (_x - 1 >= 0 && _y - 1 >= 0)
		{
			if (g_Map[_y][_x - 1].type == eBLOCKED && g_Map[_y - 1][_x - 1].type == eBLANK)
				return true;
		}
		// ㅁ
		// X O		
		if (_y + 1 < WORLD_HEIGHT && _x + 1 < WORLD_WIDTH)
		{
			if(g_Map[_y + 1][_x].type == eBLOCKED && g_Map[_y + 1][_x + 1].type == eBLANK)
				return true;
		}

		//위로 탐색
		if (findJumpNode(_x, _y - 1, eDIR_UU, _jumpX, _jumpY))
		{
			//저멀리 코너를 발견해서 좌표를 가져왔어도 지금 내 자리에다가 만들어야 하기때문에 내 좌표 넣어서 리턴
			_jumpX = _x;
			_jumpY = _y;
			return true;
		}

		//오른쪽 탐색
		if (findJumpNode(_x + 1, _y, eDIR_RR, _jumpX, _jumpY))
		{
			//저멀리 코너를 발견해서 좌표를 가져왔어도 지금 내 자리에다가 만들어야 하기때문에 내 좌표 넣어서 리턴
			_jumpX = _x;
			_jumpY = _y; 
			return true;
		}

		//자기 진행방향으로 탐색
		findJumpNode(_x + 1, _y - 1, _dir, _jumpX, _jumpY);
		break;

		//←
	case eDIR_LL:
		//o x
		//  ㅁ
		if (_y - 1 >= 0 && _x - 1 >= 0)
		{
			if (g_Map[_y - 1][_x].type == eBLOCKED && g_Map[_y - 1][_x - 1].type == eBLANK)
				return true;
		}
		//  ㅁ
		//o x
		if (_y + 1 < WORLD_HEIGHT && _x - 1 >= 0)
		{
			if (g_Map[_y + 1][_x].type == eBLOCKED && g_Map[_y + 1][_x - 1].type == eBLANK)
				return true;
		}
		
		//자기 진행방향으로 탐색
		findJumpNode(_x - 1, _y, _dir, _jumpX, _jumpY);
		break;

		//→
	case eDIR_RR:
		if (_y + 1 < WORLD_HEIGHT && _x + 1 < WORLD_WIDTH)
		{
			if (g_Map[_y + 1][_x].type == eBLOCKED && g_Map[_y + 1][_x + 1].type == eBLANK)
				return true;
		}
		if (_y - 1 > 0 && _x + 1 <= WORLD_WIDTH)
		{
			if (g_Map[_y - 1][_x].type == eBLOCKED && g_Map[_y - 1][_x + 1].type == eBLANK)
				return true;
		}

		findJumpNode(_x + 1, _y, _dir, _jumpX, _jumpY);
		break;

		//↙
	case eDIR_DL:
		if (_y - 1 >= 0 && _x - 1 >= 0)
		{
			if (g_Map[_y - 1][_x].type == eBLOCKED && g_Map[_y - 1][_x - 1].type == eBLANK)
				return true;
		}

		if (_x + 1 < WORLD_WIDTH && _y + 1 < WORLD_HEIGHT)
		{
			if (g_Map[_y][_x + 1].type == eBLOCKED && g_Map[_y + 1][_x + 1].type == eBLANK)
				return true;
		}

		if (findJumpNode(_x, _y + 1, eDIR_DD, _jumpX, _jumpY))
		{
			//저멀리 코너를 발견해서 좌표를 가져왔어도 지금 내 자리에다가 만들어야 하기때문에 내 좌표 넣어서 리턴
			_jumpX = _x;
			_jumpY = _y;
			return true;
		}

		if (findJumpNode(_x - 1, _y, eDIR_LL, _jumpX, _jumpY))
		{
			//저멀리 코너를 발견해서 좌표를 가져왔어도 지금 내 자리에다가 만들어야 하기때문에 내 좌표 넣어서 리턴
			_jumpX = _x;
			_jumpY = _y;
			return true;
		}

		findJumpNode(_x - 1, _y + 1, _dir, _jumpX, _jumpY);
		break;

		//↓
	case eDIR_DD:
		if (_x - 1 >= 0 && _y + 1 < WORLD_HEIGHT)
		{
			if (g_Map[_y][_x - 1].type == eBLOCKED && g_Map[_y + 1][_x - 1].type == eBLANK)
				return true;
		}
		if (_x + 1 < WORLD_WIDTH && _y + 1 < WORLD_HEIGHT)
		{
			if(g_Map[_y][_x + 1].type == eBLOCKED && g_Map[_y + 1][_x + 1].type == eBLANK)
				return true;
		}

		findJumpNode(_x, _y + 1, _dir, _jumpX, _jumpY);
		break;

		//↘
	case eDIR_DR:
		if (_y - 1 >=0  && _x + 1 < WORLD_WIDTH)
		{
			if (g_Map[_y - 1][_x].type == eBLOCKED && g_Map[_y - 1][_x + 1].type == eBLANK)
				return true;
		}
		if (_x - 1 >= 0 && _y + 1 < WORLD_HEIGHT)
		{
			if(g_Map[_y][_x - 1].type == eBLOCKED && g_Map[_y + 1][_x - 1].type == eBLANK)
				return true;
		}

		if (findJumpNode(_x, _y + 1, eDIR_DD, _jumpX, _jumpY))
		{
			//저멀리 코너를 발견해서 좌표를 가져왔어도 지금 내 자리에다가 만들어야 하기때문에 내 좌표 넣어서 리턴
			_jumpX = _x;
			_jumpY = _y;
			return true;
		}

		if (findJumpNode(_x + 1, _y, eDIR_RR, _jumpX, _jumpY))
		{
			//저멀리 코너를 발견해서 좌표를 가져왔어도 지금 내 자리에다가 만들어야 하기때문에 내 좌표 넣어서 리턴
			_jumpX = _x;
			_jumpY = _y;
			return true;
		}

		findJumpNode(_x + 1, _y + 1, _dir, _jumpX, _jumpY);
		break;
	}
}

void CJumpPointSearch::setEndNodeNULL()
{
	m_pEndNode = nullptr;
}

const bool FComp(const NODE* lhs, const NODE* rhs) {
	return lhs->F < rhs->F;
};

int setDir(int _dirX, int _dirY)
{
	// 0이하-왼쪽  0=같음  0이상-오른쪽
	// 0이하-위쪽  0=같음  0이상-아래쪽

	if (_dirX < 0 && _dirY < 0)		return eDIR_UL;
	if (_dirX == 0 && _dirY < 0)	return eDIR_UU;
	if (_dirX > 0 && _dirY < 0)		return eDIR_UR;
	if (_dirX < 0 && _dirY == 0)	return eDIR_LL;
	if (_dirX > 0 && _dirY == 0)	return eDIR_RR;
	if (_dirX < 0 && _dirY > 0)		return eDIR_DL;
	if (_dirX == 0 && _dirY > 0)	return eDIR_DD;
	if (_dirX > 0 && _dirY > 0)		return eDIR_DR;

	return -999;
}

bool CJumpPointSearch::pathFind(int** mapData, int _startX, int _startY, int _endX, int _endY)
{
	std::list<NODE*>::iterator iter;
	while (openList.empty() == false)
	{
		iter = openList.begin();
		delete[](*iter);
		openList.erase(iter);
	}
	while (closeList.empty() == false)
	{
		iter = closeList.begin();
		delete[](*iter);
		closeList.erase(iter);
	}

	while (fastPathList.empty() == false)
	{
		iter = fastPathList.begin();
		fastPathList.erase(iter);
	}

	std::list<POS>::iterator dotiter;
	while (line.dotList.empty() == false)
	{
		dotiter = line.dotList.begin();
		line.dotList.erase(dotiter);
	}


	for (int i = 0; i < WORLD_HEIGHT; ++i)
		for (int j = 0; j < WORLD_WIDTH; ++j)
			mapData[i][j] = eBLANK;

	setEndNodeNULL();

	//openlist에 시작 노드 추가
	NODE* newNode = nullptr;

	mapData[_startY][_startX] = eSTART;
	mapData[_endY][_endX] = eEND;

	setEndPos(_endX, _endY);
	setStartPos(_startX, _startY);

	newNode = new NODE(_startX, _startY, nullptr);

	setG(newNode);
	setH(newNode);
	setF(newNode);
	openList.push_back(newNode);
	
	while (true)
	{
		if (openList.size() == 0)
			return false;

		openList.sort(FComp);
		NODE* popNode = openList.front();
		openList.pop_front();

		//길찾기 끝
		if (popNode->ix == m_iEndX && popNode->iy == m_iEndY)
		{
			m_pEndNode = popNode;
			mapData[m_iEndY][m_iEndX] = eEND;
			mapData[m_iStartY][m_iStartX] = eSTART;

			checkPathCorrection();

			//단계별 출력때문에 마지막 한번 더 호출 그냥 없어도 됨
			//InvalidateRect(hWnd, NULL, false);
			return true;
		}

		closeList.push_back(popNode);
		int x = popNode->ix;
		int y = popNode->iy;
		mapData[y][x] = eCLOSE;


		//첫 노드면 8방향 탐색
		if (popNode->pParent == nullptr)
		{
			checkDirection(mapData, popNode, x - 1, y - 1, eDIR_UL); //↖ eDIR_UL
			checkDirection(mapData, popNode, x + 0, y - 1, eDIR_UU); //↑ eDIR_UU
			checkDirection(mapData, popNode, x + 1, y - 1, eDIR_UR); //↗ eDIR_UR
			checkDirection(mapData, popNode, x - 1, y + 0, eDIR_LL); //← eDIR_LL
			checkDirection(mapData, popNode, x + 1, y + 0, eDIR_RR); //→ eDIR_RR
			checkDirection(mapData, popNode, x - 1, y + 1, eDIR_DL); //↙ eDIR_DL
			checkDirection(mapData, popNode, x + 0, y + 1, eDIR_DD); //↓ eDIR_DD
			checkDirection(mapData, popNode, x + 1, y + 1, eDIR_DR); //↘ eDIR_DR
		}
		else
		{
			int dirX = popNode->ix - popNode->pParent->ix; // 0이하-왼쪽  0=같음  0이상-오른쪽
			int dirY = popNode->iy - popNode->pParent->iy; // 0이하-위쪽  0=같음  0이상-아래쪽 
			int dir;

			switch (setDir(dirX, dirY))
			{
				//↖
			case eDIR_UL:
				//기본 3방향
				checkDirection(mapData, popNode, x - 1, y, eDIR_LL);
				checkDirection(mapData, popNode, x - 1, y - 1, eDIR_UL);
				checkDirection(mapData, popNode, x, y - 1, eDIR_UU);

				//아래로 코너
				if (g_Map[y + 1][x].type == eBLOCKED && g_Map[y + 1][x - 1].type == eBLANK)
					checkDirection(mapData, popNode, x - 1, y + 1, eDIR_DL);

				//오른쪽으로 코너
				if (g_Map[y][x + 1].type == eBLOCKED && g_Map[y - 1][x + 1].type == eBLANK)
					checkDirection(mapData, popNode, x + 1, y - 1, eDIR_UR);
				break;

				//↑
			case eDIR_UU:
				//기본 1방향
				checkDirection(mapData, popNode, x, y - 1, eDIR_UU);

				//왼쪽으로 코너
				if (g_Map[y][x - 1].type == eBLOCKED && g_Map[y - 1][x - 1].type == eBLANK)
					checkDirection(mapData, popNode, x - 1, y - 1, eDIR_UL);

				//오른쪽으로 코너
				if (g_Map[y][x + 1].type == eBLOCKED && g_Map[y - 1][x + 1].type == eBLANK)
					checkDirection(mapData, popNode, x + 1, y - 1, eDIR_UR);
				break;

				//↗
			case eDIR_UR:
				//기본 3방향
				checkDirection(mapData, popNode, x, y - 1, eDIR_UU);
				checkDirection(mapData, popNode, x + 1, y - 1, eDIR_UR);
				checkDirection(mapData, popNode, x + 1, y, eDIR_RR);

				//왼쪽으로 코너
				if (g_Map[y][x - 1].type == eBLOCKED && g_Map[y - 1][x - 1].type == eBLANK)
					checkDirection(mapData, popNode, x - 1, y - 1, eDIR_UL);

				//아래로 코너
				if (g_Map[y + 1][x].type == eBLOCKED && g_Map[y + 1][x + 1].type == eBLANK)
					checkDirection(mapData, popNode, x + 1, y + 1, eDIR_DR);
				break;

				//←
			case eDIR_LL:
				//기본 1방향
				checkDirection(mapData, popNode, x - 1, y, eDIR_LL);

				//위로 코너
				if (g_Map[y - 1][x].type == eBLOCKED && g_Map[y - 1][x - 1].type == eBLANK)
					checkDirection(mapData, popNode, x - 1, y - 1, eDIR_UL);

				//아래로 코너
				if (g_Map[y + 1][x].type == eBLOCKED && g_Map[y + 1][x - 1].type == eBLANK)
					checkDirection(mapData, popNode, x - 1, y + 1, eDIR_DL);
				break;

				//→
			case eDIR_RR:
				//기본 1방향
				checkDirection(mapData, popNode, x + 1, y, eDIR_RR);

				//위로 코너
				if (g_Map[y - 1][x].type == eBLOCKED && g_Map[y - 1][x + 1].type == eBLANK)
					checkDirection(mapData, popNode, x + 1, y - 1, eDIR_UR);

				//아래로 코너
				if (g_Map[y + 1][x].type == eBLOCKED && g_Map[y + 1][x + 1].type == eBLANK)
					checkDirection(mapData, popNode, x + 1, y + 1, eDIR_DR);
				break;

				//↙
			case eDIR_DL:
				//기본 3방향
				checkDirection(mapData, popNode, x - 1, y, eDIR_LL);
				checkDirection(mapData, popNode, x - 1, y + 1, eDIR_DL);
				checkDirection(mapData, popNode, x, y + 1, eDIR_DD);

				//위로 코너
				if (g_Map[y - 1][x].type == eBLOCKED && g_Map[y - 1][x - 1].type == eBLANK)
					checkDirection(mapData, popNode, x - 1, y - 1, eDIR_UL);

				//오른쪽으로 코너
				if (g_Map[y][x + 1].type == eBLOCKED && g_Map[y + 1][x + 1].type == eBLANK)
					checkDirection(mapData, popNode, x + 1, y + 1, eDIR_DR);
				break;

				//↓
			case eDIR_DD:
				//기본 1방향
				checkDirection(mapData, popNode, x, y + 1, eDIR_DD);

				//왼쪽으로 코너
				if (g_Map[y][x - 1].type == eBLOCKED && g_Map[y + 1][x - 1].type == eBLANK)
					checkDirection(mapData, popNode, x - 1, y + 1, eDIR_DL);

				//오른쪽으로 코너
				if (g_Map[y][x + 1].type == eBLOCKED && g_Map[y + 1][x + 1].type == eBLANK)
					checkDirection(mapData, popNode, x + 1, y + 1, eDIR_DR);
				break;

				//↘
			case eDIR_DR:
				//기본 3방향
				checkDirection(mapData, popNode, x, y + 1, eDIR_DD);
				checkDirection(mapData, popNode, x + 1, y + 1, eDIR_DR);
				checkDirection(mapData, popNode, x + 1, y, eDIR_RR);

				//위로 코너
				if (g_Map[y - 1][x].type == eBLOCKED && g_Map[y - 1][x + 1].type == eBLANK)
					checkDirection(mapData, popNode, x + 1, y - 1, eDIR_UR);

				//왼쪽으로 코너
				if (g_Map[y][x - 1].type == eBLOCKED && g_Map[y + 1][x - 1].type == eBLANK)
					checkDirection(mapData, popNode, x - 1, y + 1, eDIR_DL);
				break;
			}
		}
	}
}

bool CJumpPointSearch::returnPos(int * x, int * y)
{
	if (line.dotList.size() == 0)
		return false;

	if (line.dotList.size() == 1)
		return true;

	line.dotList.pop_back();

	if (line.dotList.size() == 0)
		return false;

	POS pos = line.dotList.back();

	//if (pos.m_ix == m_iStartX && pos.m_iy == m_iStartY)
	//	return true;

	*x = pos.m_ix;
	*y = pos.m_iy;

	return false;
}


void CJumpPointSearch::checkPathCorrection()
{
	if (m_pEndNode == nullptr)
		return;

	NODE* temp = m_pEndNode;

	//일단 경로 옮겨담기
	while (true)
	{
		fastPathList.push_back(temp);
		temp = temp->pParent;
		if (temp == nullptr)
			break;
	}

	NODE* pStart;	//시작 노드
	NODE* pFirst;	//시작 노드의 다음
	NODE* pSecond;	//시작 노드의 다다음(하나 건너뛴 노드)
	std::list<NODE*>::iterator Iter = fastPathList.begin();
	std::list<NODE*>::iterator IterSuce;	// 성공했을때 변경하기 위한 이터레이터 위치
	std::list<NODE*>::iterator IterFail;	// 실패했을때 변경하기 위한 이터레이터 위치


	// 만든 길의 노드가 2개 이하면( 시작점, 목적지 외에 없다면 브레즌헴 알고리즘을 실행하지 않음)  
	if (fastPathList.size() <= 2)
	{
		std::list<NODE*>::iterator iter = fastPathList.begin();
		POS pos = { (*iter)->ix, (*iter)->iy };
		line.dotList.push_back(pos);
		
		iter++;
		if (iter != fastPathList.end())
		{
			if ((*iter)->ix == m_iStartX && (*iter)->iy == m_iStartY)
				return;
			POS pos = { (*iter)->ix, (*iter)->iy };
			line.dotList.push_back(pos);
		}
		return;
	}
	

	while (true)
	{
		// 1번 노드 & 성공 이터레이터
		pStart = (*Iter);
		IterSuce = Iter;
		Iter++;

		if (Iter == fastPathList.end())
			return;

		// 2번 노드 & 실패 이터레이터
		pFirst = (*Iter);
		IterFail = Iter;
		Iter++;

		if (Iter == fastPathList.end())
		{
			CBresenham temp;
			temp.setPos(pStart->ix, pStart->iy, pFirst->ix, pFirst->iy);

			// 직선을 그을 수 있다면, 중간 노드를 제거
			temp.checkDot();

			std::list<POS>::iterator iter = temp.dotList.begin();
			int size = temp.dotList.size();
			for (int i = 0; i < size; ++i)
			{
				line.dotList.push_back(*iter);
				iter++;
			}
			return;
		}
		pSecond = (*Iter);

		CBresenham temp;
		temp.setPos(pStart->ix, pStart->iy, pSecond->ix, pSecond->iy);
		
		// 직선을 그을 수 있다면, 중간 노드를 제거
		if(temp.checkDot())
		{
			std::list<POS>::iterator iter = temp.dotList.begin();
			int size = temp.dotList.size();
			for (int i = 0; i < size; ++i)
			{
				line.dotList.push_back(*iter);
				iter++;
			}


			fastPathList.remove(pFirst);
			if (fastPathList.size() <= 2)
				return;
			Iter = IterSuce;
		}
		else
		{
			std::list<POS>::iterator iter = temp.dotList.begin();
			int size = temp.dotList.size();
			for (int i = 0; i < size; ++i)
			{
				line.dotList.push_back(*iter);
				iter++;
			}

			Iter = IterFail;
		}
	}
}

//void CJumpPointSearch::pathDraw(HWND hWnd)
//{
//	if (m_pEndNode == nullptr)
//		return;
//
//	HDC hdc = GetDC(hWnd);
//
//
//	NODE* temp = m_pEndNode;
//	while (true)
//	{
//		HPEN Pen, oPen;
//		Pen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
//		oPen = (HPEN)SelectObject(hdc, Pen);
//		MoveToEx(hdc, temp->ix * 20 + 10, temp->iy * 20 + 10, NULL);
//		LineTo(hdc, temp->pParent->ix * 20 + 10, temp->pParent->iy * 20 + 10);
//		SelectObject(hdc, oPen);
//		DeleteObject(Pen);
//
//		temp = temp->pParent;
//
//		if (temp->pParent == nullptr)
//			break;
//	}
//
//	ReleaseDC(hWnd, hdc);
//}

//void CJumpPointSearch::fastPathDraw(HWND hWnd)
//{
//	if (fastPathList.size() != 0)
//	{
//		HDC hdc = GetDC(hWnd);
//		
//		std::list<NODE*>::iterator iter = fastPathList.begin();
//		std::list<NODE*>::iterator nextIter = fastPathList.begin();
//		nextIter++;
//
//		for (; nextIter != fastPathList.end(); )
//		{
//			HPEN Pen, oPen;
//			Pen = CreatePen(PS_SOLID, 3, RGB(0, 0, 255));
//			oPen = (HPEN)SelectObject(hdc, Pen);
//			MoveToEx(hdc, (*iter)->ix * 20 + 11, (*iter)->iy * 20 + 11, NULL);
//			LineTo(hdc, (*nextIter)->ix * 20 + 11, (*nextIter)->iy * 20 + 11);
//			SelectObject(hdc, oPen);
//			DeleteObject(Pen);
//
//			iter++;
//			nextIter++;
//		}
//	
//		ReleaseDC(hWnd, hdc);
//	}
//
//}
