#pragma once
#include <list>
#include "CBresenham.h"

struct NODE
{
	int ix;
	int iy;
	NODE* pParent;

	double	F;	//G+H
	double	G;	//출발점
	int		H;	//목적지와 거리


	NODE(int x, int y, NODE* parent)
	{
		ix = x;
		iy = y;
		pParent = parent;
	}
};


class CJumpPointSearch
{
public:
	std::list<NODE*> openList;
	std::list<NODE*> closeList;

	NODE* m_pEndNode;

	int m_iStartX;
	int m_iStartY;

	int m_iEndX;
	int m_iEndY;

	CBresenham line;
public:
	//길찾기 함수
	bool pathFind(int** mapData, int _startX, int _startY, int _endX, int _endY);
	bool returnPos(int* x, int* y);

public:
	CJumpPointSearch();
	~CJumpPointSearch();

	//직선
	void setG(NODE* _node);
	//대각선
	void setG_dia(NODE* _node);

	void setH(NODE* _node);
	void setF(NODE* _node);

	void setEndPos(int x, int y);
	void setStartPos(int x, int y);

	void setEndNodeNULL();

	//x,y좌표의 칸이 막힌칸인가?
	bool checkWalkAble(int _x, int _y);

	//이 방향으로 쭉 탐색. 코너가 있는가? 아니면 저 멀리 코너가 있는가?
	void checkDirection(int** mapData, NODE* _pNode, int _x, int _y, int dir);

	//checkDirection 에서 노드를 찾는 함수
	bool findJumpNode(int _x, int _y, int _dir, int& _jumpX, int& _jumpY);

	//void pathDraw(HWND hWnd);
	//void fastPathDraw(HWND hWnd);

public:
	//직선보정
	void checkPathCorrection();
	std::list<NODE*> fastPathList;


};

