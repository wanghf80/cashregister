
#include "stdafx.h"
#include <string.h>
#include <stdlib.h>


/*******************
  macro define
*******************/

#define COMMODITY_BARCODE_LEN	32
#define COMMODITY_NAME_LEN		128
#define QUANTITY_UNIT_LEN		16
#define	COMMODITY_TEMP_STR_LEN	64
#define	CSV_FILE_NAME				256

/* csv config file for commodity */
#define COMMODITY_FAVORABLE_PRICE_INFO_FILE1	"commodity_config_info_1.csv"
#define COMMODITY_FAVORABLE_PRICE_INFO_FILE2	"commodity_config_info_2.csv"
#define COMMODITY_FAVORABLE_PRICE_INFO_FILE3	"commodity_config_info_3.csv"
#define COMMODITY_FAVORABLE_PRICE_INFO_FILE4	"commodity_config_info_4.csv"

#define	LINE_LEN				4096
#define DELIMS					"," 
#define DELIM_COUNT				4

#define RETURN_SUCCESS		0
#define RETURN_ERROR		1


#define FAVORABLE_FLAG_2GET1FREE		0x01
#define FAVORABLE_FLAG_95				0x02

#define FAVORABLE_NUMBER	0.95

#define ERR_MSG_OPEN_CSVFILE_FAILED		"Failed to open csv config file!"
#define ERR_MSG_MALLOC_FAILED			"Failed to malloc!"
#define ERR_MSG_UNEXPECT_ERR			"Unexpect error!"


/* output format */
#define OUTPUT_FORMAT_TITLE					"***<没钱赚商店>购物清单***\n"
#define OUTPUT_FORMAT_HALVING_LINE			"----------------------\n"
#define	OUTPUT_FORMAT_BUY2GET1FREE_TITLE	"买二赠一商品：\n"
#define OUTPUT_FORMAT_ENDLINE				"**********************\n"
#define	OUTPUT_LINE_LEN						1024

#define	OUTPUT_SHOPPING_LIST		"名称：%s，数量：%d%s，单价：%.2f(元)，小计：%.2f(元)\n"
#define	OUTPUT_SHOPPING_LIST_95		"名称：%s，数量：%d%s，单价：%.2f(元)，小计：%.2f(元), 节省%.2f(元)\n"
#define	OUTPUT_BYE2GET1FREE_LIST	"名称：%s，数量：%d%s\n"
#define	OUTPUT_TOTAL_AMOUNT			"总计：%.2f(元)\n"
#define	OUTPUT_TOTAL_SAVE_COSTS		"节省：%.2f(元)\n"


/**************************************************************************
    Macro Function Name:   GETBARCODEANDNUMFROMSTRING()
***************************************************************************
    Function General:  Get bar code and number informationt from input data.
    
    Paramter		: pStrTmp(IN)
						: input data
					  szBarCode(OUT)
						: the bar code of bought commodity 
					  uiNumber(OUT)
					    : the number of bought commodity                           
                                    
    Notice: none

	Date:  2016/03/09
**************************************************************************/
#define GETBARCODEANDNUMFROMSTRING(pStrTmp, szBarCode, uiNumber) \
{\
	char *p = NULL;\
	char *pStr = NULL; \
	char szTmp[COMMODITY_TEMP_STR_LEN];\
	\
	memset(szTmp, 0, sizeof(szTmp));\
	sscanf(pStrTmp, "%*[^']'%s", szTmp);\
	p = strchr(szTmp, '\'');\
	*p = '\0';\
	\
	/* split bar code and num */\
	pStr = strchr(szTmp, '-');\
	if(pStr != NULL)\
	{ /* - is exist */\
		(*uiNumber) = atoi(pStr+1); /* get number */\
		*pStr = '\0';\
		strcpy(szBarCode, szTmp); /* get bar code */\
	}\
	else\
	{ /* - is not exist */\
		strcpy(szBarCode, szTmp); /* get bar code */\
		(*uiNumber) = 1;	/* set number to 1 */\
	}\
}


/**************************************************************************
    Macro Function Name:   GET_TOTAL_COSTS()
***************************************************************************
    Function General:  Get total costs and save costs for commodity.
    
    Paramter		: fUnitPrice(IN)
						: unit price 
					  uiNumber(IN)
					    : bought amount
					  iFlag(IN)
					    : the bit flag of favorable price 
					  fTotalCosts(OUT)
						: the bar code of bought commodity 
					  fCommoditySaveCosts(OUT)
					    : the number of bought commodity                           
                                    
    Notice: none

	Date:  2016/03/09
**************************************************************************/
#define GET_TOTAL_COSTS(fUnitPrice, uiNumber, iFlag, fTotalCosts, fCommoditySaveCosts)	\
{                                                       \
	if(FAVORABLE_FLAG_2GET1FREE == (iFlag & FAVORABLE_FLAG_2GET1FREE))		\
	{ \
		*fTotalCosts = fUnitPrice*(uiNumber - uiNumber/3);	\
		*fCommoditySaveCosts = fUnitPrice*uiNumber - (*fTotalCosts);	\
	} \
	else if(FAVORABLE_FLAG_95 == (iFlag & FAVORABLE_FLAG_95))	\
	{ \
		*fTotalCosts = fUnitPrice*uiNumber*FAVORABLE_NUMBER;	\
		*fCommoditySaveCosts = fUnitPrice*uiNumber - (*fTotalCosts);	\
	} \
	else \
	{ \
		*fTotalCosts = fUnitPrice*uiNumber;	\
		*fCommoditySaveCosts = 0.0;	\
	} \
}



/****************************
 struct define
****************************/

/*********************************
 set by clerk in cash register 
*********************************/

/* Commodity favorable price information */
typedef struct _COMMODITY_FAVORABLE_PRICE_INFO {
	unsigned char	szBarCode[COMMODITY_BARCODE_LEN];	/* bar code */
	unsigned char	szName[COMMODITY_NAME_LEN];			/* name of commodity */
	float			fUnitPrice;							/* unit price */
	unsigned char	szQuantityUnit[QUANTITY_UNIT_LEN];	/* quantity unit */
	unsigned char	ucFavorableBitFlag;					/*  bit7~bit2: rfu    
																 bit1: 0.95 favorable price		1->enalbe  0->disable
																 bit0: buy two get one free		1->enable  0->disable
	unsigned char	ucRfu[3];							/* reserve for future */
}COMMODITY_FAVORABLE_PRICE_INFO;

/* Commodity favorable price information list */
typedef struct _COMMODITY_FAVORABLE_PRICE_INFO_LIST {
	COMMODITY_FAVORABLE_PRICE_INFO				sCommodityNode;		/* commdity info node */
	struct _COMMODITY_FAVORABLE_PRICE_INFO_LIST	*pNext;				/* next node */
}COMMODITY_FAVORABLE_PRICE_INFO_LIST;



/****************************************
 bought commodity detailed list information
*****************************************/

/* Commodity information */
typedef struct _COMMODITY_INFO {
	COMMODITY_FAVORABLE_PRICE_INFO	sCommodityBaseInfo;					/* commodity base info */
	unsigned int					uiNumber;							/* number of commodity */
	float							fTotal;								/* total costs */
	float							fSaveCosts;							/* save costs */

}COMMODITY_INFO;


/* All commodity information list */
typedef struct _COMMODITY_INFO_LIST {
	COMMODITY_INFO				sCommodityNode;		/* commdity info node */
	struct _COMMODITY_INFO_LIST	*pNext;				/* next node */
}COMMODITY_INFO_LIST;


/* Commodity(buy two get one free) information */
typedef struct _BUY2GET1FREE_COMMODITY_INFO {
	unsigned char	szName[COMMODITY_NAME_LEN];			/* name of commodity */
	unsigned int	uiNumber;							/* number of commodity */
	unsigned char	szQuantityUnit[QUANTITY_UNIT_LEN];	/* quantity unit */
}BUY2GET1FREE_COMMODITY_INFO;


/* Commodity(buy two get one free) information list */
typedef struct _BUY2GET1FREE_COMMODITY_INFO_LIST {
	BUY2GET1FREE_COMMODITY_INFO					sCommodityInfo;			/* name of buy2get1free commodity node */
	struct  _BUY2GET1FREE_COMMODITY_INFO_LIST	*pNext;					/* next node */
}BUY2GET1FREE_COMMODITY_INFO_LIST;


/* Commodity total amount information */
typedef struct _COMMODITY_TOTAL_AMOUNT_INFO {
	float	fTotalAmount;		/* total amount infomation of all commodity */
	float	fSaveCosts;			/* save costs of all commodity */
}COMMODITY_TOTAL_AMOUNT_INFO;




/**************************************************************************
    Function Name:   GetCommodityFavorablePriceInfoFromCSV()
***************************************************************************
    Function General:  Get the base information of commodity from csv config file.
    
    Paramter		: const char *pFileName(IN)
						: csv file name
					  COMMODITY_FAVORABLE_PRICE_INFO_LIST **pCommodityFavorablePriceInfoHead(OUT)
						: the config information of commodity 

    Return Value	: int  iRet  0 : Normal
                                 1 : Abnormal                                  
                                    
    Notice: none

	Date:  2016/03/09
**************************************************************************/

int GetCommodityFavorablePriceInfoFromCSV(const char *pFileName, COMMODITY_FAVORABLE_PRICE_INFO_LIST **pCommodityFavorablePriceInfoHead)
{
	int iRet = RETURN_SUCCESS;
	FILE *fp = NULL;
	char szLineBuffer[LINE_LEN];
	char *pStrTmp = NULL;
	COMMODITY_FAVORABLE_PRICE_INFO_LIST *pNodeTemp = NULL;	/*  the pointer of new node */
	int i = 0;

	memset(szLineBuffer, 0, sizeof(szLineBuffer));

	if(NULL == pCommodityFavorablePriceInfoHead)
	{
		fprintf(stderr, ERR_MSG_UNEXPECT_ERR);
		goto L_Err;
	}

	/* open the csv file */
	fp = fopen(pFileName, "r");
	if(NULL == fp)
	{
		fprintf(stderr, ERR_MSG_OPEN_CSVFILE_FAILED);
		goto L_Err;
	}

	/* read csv file */
	while(NULL != fgets(szLineBuffer, sizeof(szLineBuffer), fp))
	{   
		/* malloc a new node */
		pNodeTemp = (COMMODITY_FAVORABLE_PRICE_INFO_LIST *)malloc(sizeof(COMMODITY_FAVORABLE_PRICE_INFO_LIST));
		if(NULL == pNodeTemp)
		{
			fprintf(stderr, ERR_MSG_MALLOC_FAILED);
			goto L_Err;
		}

		memset(pNodeTemp, 0, sizeof(COMMODITY_FAVORABLE_PRICE_INFO_LIST));


		/* set data to new node */
		pStrTmp = strtok(szLineBuffer, DELIMS);
		
		if(NULL != pStrTmp)
		{
			/* get commodity bar code */
			strcpy((char *)pNodeTemp->sCommodityNode.szBarCode, pStrTmp);
		}

		/* parse the information of a line */
		for(i = 0; i<DELIM_COUNT; i++)
		{
			pStrTmp = strtok(NULL, DELIMS);
		
			if(NULL != pStrTmp)
			{
				switch(i)
				{
					case 0:
						/* get commodity name */
						strcpy((char *)pNodeTemp->sCommodityNode.szName, pStrTmp);
						break;

					case 1:
						/* get commodity unit price */
						pNodeTemp->sCommodityNode.fUnitPrice = atof(pStrTmp);
						break;

					case 2:
						/* get commodity favorable price flag */
						strcpy((char *)pNodeTemp->sCommodityNode.szQuantityUnit, pStrTmp);
						break;

					case 3:
						/* get commodity favorable price flag */
						pNodeTemp->sCommodityNode.ucFavorableBitFlag = atoi(pStrTmp);
						break;

					default:
						break;
				}
			}
		}


		/* Insert new node to linked list */
		if(NULL == (*pCommodityFavorablePriceInfoHead))
		{
			(*pCommodityFavorablePriceInfoHead) = pNodeTemp;
			(*pCommodityFavorablePriceInfoHead)->pNext = NULL;
		}
		else
		{
			pNodeTemp->pNext = (*pCommodityFavorablePriceInfoHead)->pNext;
			(*pCommodityFavorablePriceInfoHead)->pNext = pNodeTemp;
		}

	}


L_Exit:
	if(NULL != fp)
	{
		fclose(fp);
		fp = NULL;
	}

	return iRet;

L_Err:
	iRet = RETURN_ERROR;
	goto L_Exit;
}



/**************************************************************************
    Function Name:   FreeCommodityFavorablePriceInfoList()
***************************************************************************
    Function General: Free the memory of commodity config information
    
    Paramter		: 
					  COMMODITY_FAVORABLE_PRICE_INFO_LIST **pCommodityFavorablePriceInfoHead(IN)
						: the config information of commodity 

    Return Value	: void                               
                                    
    Notice: none

	Date:  2016/03/09
**************************************************************************/
void FreeCommodityFavorablePriceInfoList(COMMODITY_FAVORABLE_PRICE_INFO_LIST **pCommodityFavorablePriceInfoHead)
{
	COMMODITY_FAVORABLE_PRICE_INFO_LIST *pWorkNode = NULL;
	COMMODITY_FAVORABLE_PRICE_INFO_LIST *pNextNode = NULL;

	if(NULL == pCommodityFavorablePriceInfoHead || NULL == *pCommodityFavorablePriceInfoHead)
	{
		goto L_Exit;
	}

	pWorkNode = *pCommodityFavorablePriceInfoHead;

	while(1)
	{
		if(NULL == pWorkNode)
		{
			break;
		}

		pNextNode = pWorkNode->pNext;
		free(pWorkNode);
		pWorkNode = pNextNode;
	}

	*pCommodityFavorablePriceInfoHead = NULL;

L_Exit:
	return;
}



/**************************************************************************
    Function Name:   FreeBuy2Get1FreeInfoList()
***************************************************************************
    Function General:  Free the memory of Bye 2 get 1 free information list.
    
    Paramter		: 
					  BUY2GET1FREE_COMMODITY_INFO_LIST **pBuy2Get1FreeCommodityListHead(IN)
						: buy 2 get 1 free information of commodity 

    Return Value	: void                                
                                    
    Notice: none

	Date:  2016/03/09
**************************************************************************/
void FreeBuy2Get1FreeInfoList(BUY2GET1FREE_COMMODITY_INFO_LIST **pBuy2Get1FreeCommodityListHead)
{
	BUY2GET1FREE_COMMODITY_INFO_LIST *pWorkNode = NULL;
	BUY2GET1FREE_COMMODITY_INFO_LIST *pNextNode = NULL;

	if(NULL == pBuy2Get1FreeCommodityListHead || NULL == *pBuy2Get1FreeCommodityListHead)
	{
		goto L_Exit;
	}

	pWorkNode = *pBuy2Get1FreeCommodityListHead;

	while(1)
	{
		if(NULL == pWorkNode)
		{
			break;
		}

		pNextNode = pWorkNode->pNext;
		free(pWorkNode);
		pWorkNode = pNextNode;
	}

	*pBuy2Get1FreeCommodityListHead = NULL;

L_Exit:
	return;
}


/**************************************************************************
    Function Name:   FreeCommodityInfoList()
***************************************************************************
    Function General: Free the memory of commodity information
    
    Paramter		: 
					  COMMODITY_INFO_LIST **pCommodityInfoListHead(IN)
						: the information of commodity 

    Return Value	: void                               
                                    
    Notice: none

	Date:  2016/03/09
**************************************************************************/
void FreeCommodityInfoList(COMMODITY_INFO_LIST **pCommodityInfoListHead)
{
	COMMODITY_INFO_LIST *pWorkNode = NULL;
	COMMODITY_INFO_LIST *pNextNode = NULL;

	if(NULL == pCommodityInfoListHead || NULL == *pCommodityInfoListHead)
	{
		goto L_Exit;
	}

	pWorkNode = *pCommodityInfoListHead;

	while(1)
	{
		if(NULL == pWorkNode)
		{
			break;
		}

		pNextNode = pWorkNode->pNext;
		free(pWorkNode);
		pWorkNode = pNextNode;
	}

	*pCommodityInfoListHead = NULL;

L_Exit:
	return;
}



/**************************************************************************
    Function Name:   InsertNodeToBuy2Get1FreeInfoList()
***************************************************************************
    Function General:  Insert the new node information of buy 2 get 1 free commodity to linked list.
    
    Paramter		: COMMODITY_INFO_LIST *pNodeInfo(IN)
						: the new node information
					  BUY2GET1FREE_COMMODITY_INFO_LIST **pOutBuy2Get1FreeCommodityList(OUT)
						: the pointer of linked list

    Return Value	: int  iRet  0 : Normal
                                 1 : Abnormal                                  
                                    
    Notice: none

	Date:  2016/03/09
**************************************************************************/
int InsertNodeToBuy2Get1FreeInfoList(COMMODITY_INFO_LIST *pNodeInfo, BUY2GET1FREE_COMMODITY_INFO_LIST **pOutBuy2Get1FreeCommodityList)
{
	int iRet = RETURN_SUCCESS;
	BUY2GET1FREE_COMMODITY_INFO_LIST	*pNewNode = NULL;	/* the pointer of new node */


	/* parameter check */
	if(NULL == pNodeInfo
	 ||NULL == pOutBuy2Get1FreeCommodityList)
	{
		fprintf(stderr, ERR_MSG_UNEXPECT_ERR);
		goto L_Err;
	}


	/* malloc a new node */
	pNewNode = (BUY2GET1FREE_COMMODITY_INFO_LIST *)malloc(sizeof(BUY2GET1FREE_COMMODITY_INFO_LIST));
	if(NULL == pNewNode)
	{
		fprintf(stderr, ERR_MSG_MALLOC_FAILED);
		goto L_Err;
	}

	memset(pNewNode, 0, sizeof(BUY2GET1FREE_COMMODITY_INFO_LIST));

	/* set commodity name info */
	memcpy(pNewNode->sCommodityInfo.szName, pNodeInfo->sCommodityNode.sCommodityBaseInfo.szName, COMMODITY_NAME_LEN);

	/* set commodity save costs(number) */
	pNewNode->sCommodityInfo.uiNumber = pNodeInfo->sCommodityNode.uiNumber/3;
	
	/* set commodity quantity unit */
	memcpy(pNewNode->sCommodityInfo.szQuantityUnit, pNodeInfo->sCommodityNode.sCommodityBaseInfo.szQuantityUnit, QUANTITY_UNIT_LEN);
	

	/* Insert new node to linked list */
	if(NULL == (*pOutBuy2Get1FreeCommodityList))
	{
		(*pOutBuy2Get1FreeCommodityList) = pNewNode;
		(*pOutBuy2Get1FreeCommodityList)->pNext = NULL;
	}
	else
	{
		pNewNode->pNext = (*pOutBuy2Get1FreeCommodityList)->pNext;
		(*pOutBuy2Get1FreeCommodityList)->pNext = pNewNode;
	}



L_Exit:

	return iRet;

L_Err:
	iRet = RETURN_ERROR;
	goto L_Exit;
}




/**************************************************************************
    Function Name:   GetCommodityInfoList()
***************************************************************************
    Function General:  Get all commodity information.
    
    Paramter		: const char *pInputCommodityInfo(IN)
						: the input data of commodity
					  COMMODITY_FAVORABLE_PRICE_INFO_LIST *pCommodityFavorablePriceInfoHead(OUT)
						: the base information of commodity
					  COMMODITY_INFO_LIST **pOutCommodityInfoList(OUT)
					    : the information of all commodity
					  BUY2GET1FREE_COMMODITY_INFO_LIST **pOutBue2Get1FreeCommodityList(OUT)
					    : the information of buy 2 get 1 free commodity
					  COMMODITY_TOTAL_AMOUNT_INFO	*pCommodityTotalAmountInfo(OUT)
					    :  the information of total amount.

    Return Value	: int  iRet  0 : Normal
                                 1 : Abnormal                                  
                                    
    Notice: none

	Date:  2016/03/09
**************************************************************************/

int GetCommodityInfoList(const char *pInputCommodityInfo, 
						   COMMODITY_FAVORABLE_PRICE_INFO_LIST *pCommodityFavorablePriceInfoHead,
						   COMMODITY_INFO_LIST **pOutCommodityInfoList,
						   BUY2GET1FREE_COMMODITY_INFO_LIST **pOutBue2Get1FreeCommodityList,
						   COMMODITY_TOTAL_AMOUNT_INFO	*pCommodityTotalAmountInfo)
{
	int iRet = RETURN_SUCCESS;
	int iFuncRet = RETURN_SUCCESS;
	char szBarCode[COMMODITY_BARCODE_LEN];					/* bar code */
	unsigned int	uiNumber = 0;;							/* number of commodity */
	COMMODITY_FAVORABLE_PRICE_INFO_LIST	*pWorkNode = NULL;	/* the work pointer of COMMODITY_FAVORABLE_PRICE_INFO_LIST */
	COMMODITY_INFO_LIST	*pNodeTemp = NULL;
	COMMODITY_INFO_LIST	*pCommodityInfoWorkNode2 = NULL;	/* the work pointer of COMMODITY_INFO_LIST */
	float fCommodityCosts = 0.00;							/* the value of total costs */
	float fCommoditySaveCosts = 0.00;						/* the value of save costs */
	char *pStrTmp = NULL;
	unsigned char ucExistFlag = 0;							/* flag: same bar code is exist?   1:exist 0:does not exist */

	memset(szBarCode, 0, sizeof(szBarCode));

	/* parameter check */
	if(NULL == pCommodityFavorablePriceInfoHead
	 ||NULL == pOutCommodityInfoList
	 ||NULL == pOutBue2Get1FreeCommodityList)
	{
		fprintf(stderr, ERR_MSG_UNEXPECT_ERR);
		goto L_Err;
	}


	/* parse input string data */
	pStrTmp = strtok((char *)pInputCommodityInfo, DELIMS);

	while(pStrTmp != NULL)
	{
		/* get bar code and number from intput string data */
		GETBARCODEANDNUMFROMSTRING(pStrTmp, szBarCode, &uiNumber);

		/* search commodity base infomation by bar code */
		for(pWorkNode = pCommodityFavorablePriceInfoHead; pWorkNode != NULL; pWorkNode = pWorkNode->pNext)
		{
			if(0 == strcmp((const char*)szBarCode, (char *)pWorkNode->sCommodityNode.szBarCode))
			{ /* find bar code */
				/* check same bar code in commodity info list */
				for(pCommodityInfoWorkNode2 = (*pOutCommodityInfoList); pCommodityInfoWorkNode2 != NULL; pCommodityInfoWorkNode2 = pCommodityInfoWorkNode2->pNext)
				{
					if(0 == strcmp((const char*)szBarCode, (char *)pCommodityInfoWorkNode2->sCommodityNode.sCommodityBaseInfo.szBarCode))
					{ /* found same bar code in commodity info list */
						pCommodityInfoWorkNode2->sCommodityNode.uiNumber += uiNumber; /* add num of commodity */
						ucExistFlag = 1;
						break;
					}
				}
		
				if(0 == ucExistFlag)
				{  /* if same bar code is not exist in commodity info list */
					
					/* get commodity info list */
					/* malloc a new node */
					pNodeTemp = (COMMODITY_INFO_LIST *)malloc(sizeof(COMMODITY_INFO_LIST));
					if(NULL == pNodeTemp)
					{
						fprintf(stderr, ERR_MSG_MALLOC_FAILED);
						goto L_Err;
					}

					memset(pNodeTemp, 0, sizeof(COMMODITY_INFO_LIST));

					/* set commodity base info */
					memcpy(&(pNodeTemp->sCommodityNode.sCommodityBaseInfo), &(pWorkNode->sCommodityNode), sizeof(COMMODITY_FAVORABLE_PRICE_INFO));

					/* set commodity number */
					pNodeTemp->sCommodityNode.uiNumber = uiNumber;
						
					/* Insert new node to linked list */
					if(NULL == (*pOutCommodityInfoList))
					{
						(*pOutCommodityInfoList) = pNodeTemp;
						(*pOutCommodityInfoList)->pNext = NULL;
					}
					else
					{
						pNodeTemp->pNext = (*pOutCommodityInfoList)->pNext;
						(*pOutCommodityInfoList)->pNext = pNodeTemp;
					}
				} /* end if */

				break;
			} /* end if */
		} /* end for */

		ucExistFlag = 0; /* rest flag to 0 */
		/* get next string data */
		pStrTmp = strtok(NULL, DELIMS);
	} /* end while */



	/* Loop all commodity info list, and set commodity costs and saved costs info */
	for(pCommodityInfoWorkNode2 = (*pOutCommodityInfoList); pCommodityInfoWorkNode2 != NULL; pCommodityInfoWorkNode2 = pCommodityInfoWorkNode2->pNext)
	{
		/* get commodity costs and saved costs */
		GET_TOTAL_COSTS(pCommodityInfoWorkNode2->sCommodityNode.sCommodityBaseInfo.fUnitPrice, 
						pCommodityInfoWorkNode2->sCommodityNode.uiNumber, 
						pCommodityInfoWorkNode2->sCommodityNode.sCommodityBaseInfo.ucFavorableBitFlag, 
						&fCommodityCosts,
						&fCommoditySaveCosts);

		pCommodityInfoWorkNode2->sCommodityNode.fTotal = fCommodityCosts;
		pCommodityInfoWorkNode2->sCommodityNode.fSaveCosts = fCommoditySaveCosts;



		/* set total amount information to pCommodityTotalAmountInfo */
		(*pCommodityTotalAmountInfo).fTotalAmount += fCommodityCosts;		/* total costs of all commodity */
		(*pCommodityTotalAmountInfo).fSaveCosts += fCommoditySaveCosts;		/* total save costs of all commodity */


		/* get bue two get one free info list */
		if(FAVORABLE_FLAG_2GET1FREE == (pCommodityInfoWorkNode2->sCommodityNode.sCommodityBaseInfo.ucFavorableBitFlag & FAVORABLE_FLAG_2GET1FREE))
		{
			/* when buy 2 get 1 free flag is set */
			iFuncRet = InsertNodeToBuy2Get1FreeInfoList(pCommodityInfoWorkNode2, pOutBue2Get1FreeCommodityList);
			if(RETURN_SUCCESS != iFuncRet)
			{
				goto L_Err;
			}
		}
	} /* end for loop */


L_Exit:

	return iRet;

L_Err:
	iRet = RETURN_ERROR;
	goto L_Exit;
}




/**************************************************************************
    Function Name:   OutShoppintList()
***************************************************************************
    Function General:  Output all shoppint list.
    
    Paramter		: 
					  COMMODITY_INFO_LIST *pOutCommodityInfoList(IN)
					    :  the information of all commdotiy

    Return Value	: void                              
                                    
    Notice: none

	Date:  2016/03/09
**************************************************************************/
void OutShoppintList(COMMODITY_INFO_LIST *pOutCommodityInfoList)
{
	COMMODITY_INFO_LIST *pWorkNode = NULL;
	char szOutBuffer[OUTPUT_LINE_LEN];

	if(NULL == pOutCommodityInfoList)
	{
		goto L_Exit;
	}

	/* output title */
	printf(OUTPUT_FORMAT_TITLE);

	/* output shoppint list */
	for(pWorkNode = pOutCommodityInfoList; pWorkNode != NULL; pWorkNode = pWorkNode->pNext)
	{
		memset(szOutBuffer, 0, sizeof(szOutBuffer));

		if(FAVORABLE_FLAG_95 == (pWorkNode->sCommodityNode.sCommodityBaseInfo.ucFavorableBitFlag & FAVORABLE_FLAG_95))
		{  /* If have 0.95 favorable price, output save costs */
			sprintf(szOutBuffer, OUTPUT_SHOPPING_LIST_95, 
				pWorkNode->sCommodityNode.sCommodityBaseInfo.szName,
				pWorkNode->sCommodityNode.uiNumber,
				pWorkNode->sCommodityNode.sCommodityBaseInfo.szQuantityUnit,
				pWorkNode->sCommodityNode.sCommodityBaseInfo.fUnitPrice,
				pWorkNode->sCommodityNode.fTotal,
				pWorkNode->sCommodityNode.fSaveCosts);
		}
		else
		{
			sprintf(szOutBuffer, OUTPUT_SHOPPING_LIST, 
				pWorkNode->sCommodityNode.sCommodityBaseInfo.szName,
				pWorkNode->sCommodityNode.uiNumber,
				pWorkNode->sCommodityNode.sCommodityBaseInfo.szQuantityUnit,
				pWorkNode->sCommodityNode.sCommodityBaseInfo.fUnitPrice,
				pWorkNode->sCommodityNode.fTotal);
		}

		printf(szOutBuffer);
	}


L_Exit:
	return;
}


/**************************************************************************
    Function Name:   OutBuy2Get1FreeCommodityList()
***************************************************************************
    Function General:  Output all buy two get one free information list.
    
    Paramter		: 
					  BUY2GET1FREE_COMMODITY_INFO_LIST *pOutBue2Get1FreeCommodityList(IN)
					    :  the information of all commdotiy(buy two get one free)

    Return Value	: void                              
                                    
    Notice: none

	Date:  2016/03/09
**************************************************************************/
void OutBuy2Get1FreeCommodityList( BUY2GET1FREE_COMMODITY_INFO_LIST *pOutBue2Get1FreeCommodityList)
{
	BUY2GET1FREE_COMMODITY_INFO_LIST *pWorkNode = NULL;
	char szOutBuffer[OUTPUT_LINE_LEN];

	if(NULL == pOutBue2Get1FreeCommodityList)
	{
		goto L_Exit;
	}

	/* output halving line and title */
	printf(OUTPUT_FORMAT_HALVING_LINE);
	printf(OUTPUT_FORMAT_BUY2GET1FREE_TITLE);

	/* output bue two get one free commodity list */
	for(pWorkNode = pOutBue2Get1FreeCommodityList; pWorkNode != NULL; pWorkNode = pWorkNode->pNext)
	{
		memset(szOutBuffer, 0, sizeof(szOutBuffer));

		sprintf(szOutBuffer, OUTPUT_BYE2GET1FREE_LIST, 
					pWorkNode->sCommodityInfo.szName, 
					pWorkNode->sCommodityInfo.uiNumber,
					pWorkNode->sCommodityInfo.szQuantityUnit);

		printf(szOutBuffer);
	}


L_Exit:
	return;
}


/**************************************************************************
    Function Name:   OutTotalAmountInfo()
***************************************************************************
    Function General:  Output the total amount of all commodity.
    
    Paramter		: 
					  COMMODITY_TOTAL_AMOUNT_INFO *pOutCommodityTotalAmountInfo(IN)
					    :  the information of total amount

    Return Value	: void                              
                                    
    Notice: If the value of save costs is 0, do not output save costs information.

	Date:  2016/03/09
**************************************************************************/
void OutTotalAmountInfo(COMMODITY_TOTAL_AMOUNT_INFO *pOutCommodityTotalAmountInfo)
{
	char szOutBuffer[OUTPUT_LINE_LEN];

	if(NULL == pOutCommodityTotalAmountInfo)
	{
		goto L_Exit;
	}

	/* output halving line */
	printf(OUTPUT_FORMAT_HALVING_LINE);

	/* output total amount */
	memset(szOutBuffer, 0, sizeof(szOutBuffer));
	sprintf(szOutBuffer, OUTPUT_TOTAL_AMOUNT, (*pOutCommodityTotalAmountInfo).fTotalAmount);
	printf(szOutBuffer);

	/* output save costs */
	if((*pOutCommodityTotalAmountInfo).fSaveCosts > -0.000001 && (*pOutCommodityTotalAmountInfo).fSaveCosts < 0.000001)
	{	/* if save costs is 0.00, do not output */
		goto L_OutEnd;
	}
	memset(szOutBuffer, 0, sizeof(szOutBuffer));
	sprintf(szOutBuffer, OUTPUT_TOTAL_SAVE_COSTS, (*pOutCommodityTotalAmountInfo).fSaveCosts);
	printf(szOutBuffer);

L_OutEnd:
	/* output end line */
	printf(OUTPUT_FORMAT_ENDLINE);

L_Exit:
	return;
}


/**************************************************************************
    Function Name:   OutCommodityInfoList()
***************************************************************************
    Function General:  Output the information of all commodity.
    
    Paramter		: COMMODITY_INFO_LIST *pOutCommodityInfoList(IN)
	                    :  the detail information of all commodity
					  BUY2GET1FREE_COMMODITY_INFO_LIST *pOutBue2Get1FreeCommodityList(IN)
					    :  the informaiont of commodity(buy two get non free)
					  COMMODITY_TOTAL_AMOUNT_INFO *pOutCommodityTotalAmountInfo(IN)
					    :  the information of total amount

    Return Value	: void                              
                                    
    Notice: none

	Date:  2016/03/09
**************************************************************************/
int OutCommodityInfoList(COMMODITY_INFO_LIST *pOutCommodityInfoList,
						 BUY2GET1FREE_COMMODITY_INFO_LIST *pOutBue2Get1FreeCommodityList,
						 COMMODITY_TOTAL_AMOUNT_INFO *pOutCommodityTotalAmountInfo)
{
	int iRet = RETURN_SUCCESS;
	int iFuncRet = RETURN_SUCCESS;

	/******************************
	 Output shopping list
	*******************************/
	OutShoppintList(pOutCommodityInfoList);

	/*******************************************
	 Output but two get one free commodity list
	*******************************************/
	OutBuy2Get1FreeCommodityList(pOutBue2Get1FreeCommodityList);

	/*****************************************
	 Output total amount information
	******************************************/
	OutTotalAmountInfo(pOutCommodityTotalAmountInfo);

L_Exit:
	return iRet;

L_Err:
	iRet = RETURN_ERROR;
	goto L_Exit;
}



/**************************************************************************
    Function Name:   _tmain()
***************************************************************************
    Function General:  main
    
    Paramter		: int argc
	                    :  the number of argument 
					  _TCHAR* argv[]
					    :  argument list

    Return Value	: int  iRet  0 : Normal
                                 1 : Abnormal                             
                                    
    Notice: none

	Date:  2016/03/09
**************************************************************************/
int main(int argc, char *argv[])
{
	int iRet = RETURN_SUCCESS;
	int iFuncRet = RETURN_SUCCESS;
	char szFileName[CSV_FILE_NAME];		/* csv file name */
	int	iArgForFileNameConf = 0;		/* csv file by arg */
	char * pInputData = NULL;			/* the pointer of intpu data */
	char szInputDataDefault[] = \
	"[\
		'ITEM000001',\
		'ITEM000001',\
		'ITEM000001',\
		'ITEM000001',\
		'ITEM000001',\
		'ITEM000003-2',\
		'ITEM000005',\
		'ITEM000005',\
		'ITEM000005'\
	]";									/* the default intpu data */

	COMMODITY_FAVORABLE_PRICE_INFO_LIST *pCommodityFavorablePriceInfoHead = NULL;
	COMMODITY_INFO_LIST	*pCommodityInfoList = NULL;									/* the detail commodity information list */
	BUY2GET1FREE_COMMODITY_INFO_LIST *pBue2Get1FreeCommodityList = NULL;			/* the buy2get1free commodity information list */
	COMMODITY_TOTAL_AMOUNT_INFO	sCommodityTotalAmountInfo;							/* the total amount information */

	memset(szFileName, 0, sizeof(szFileName));
	memset(&sCommodityTotalAmountInfo, 0, sizeof(sCommodityTotalAmountInfo));

	
	/* check argument */
	if(1 == argc)
	{ /* print useage */
		printf("Usage : cashregister.exe {1-4} \"InputData\"\n");
		printf("Sample1: cashregister.exe 1\n");
		printf("Sample2: cashregister.exe 2  \"['ITEM000001-3', 'ITEM000003-2', 'ITEM000005-5']\"\n");
		goto L_Err;
	}
	else
	{ /* set csv config file name */
		iArgForFileNameConf = atoi((const char *)argv[1]);
		switch(iArgForFileNameConf)
		{
			case 1:
				strcpy(szFileName, COMMODITY_FAVORABLE_PRICE_INFO_FILE1);
				break;
			case 2:
				strcpy(szFileName, COMMODITY_FAVORABLE_PRICE_INFO_FILE2);
				break;
			case 3:
				strcpy(szFileName, COMMODITY_FAVORABLE_PRICE_INFO_FILE3);
				break;
			case 4:
				strcpy(szFileName, COMMODITY_FAVORABLE_PRICE_INFO_FILE4);
				break;
			default:
				strcpy(szFileName, COMMODITY_FAVORABLE_PRICE_INFO_FILE1);
				break;
		}

	}

	/* get commodity price info from csv file */
	iFuncRet = GetCommodityFavorablePriceInfoFromCSV(szFileName, &pCommodityFavorablePriceInfoHead);
	if(RETURN_SUCCESS != iFuncRet)
	{
		goto L_Err;
	}


	/* get intpu data */
	if(argc >= 3)
	{  	/* get input data from command line */
		pInputData = (char *)argv[2];	
	}
	else
	{   /* set default input data */
		pInputData = szInputDataDefault;
	}

	/* get commodity info list */
	iFuncRet = GetCommodityInfoList(pInputData, 
						   pCommodityFavorablePriceInfoHead,
						   &pCommodityInfoList,
						   &pBue2Get1FreeCommodityList,
						   &sCommodityTotalAmountInfo);
	if(RETURN_SUCCESS != iFuncRet)
	{
		goto L_Err;
	}

	/* free memory (commodity price info from csv file) */
	FreeCommodityFavorablePriceInfoList(&pCommodityFavorablePriceInfoHead);

	/* out commodity info list */
	iFuncRet = OutCommodityInfoList(pCommodityInfoList, pBue2Get1FreeCommodityList, &sCommodityTotalAmountInfo);
	if(RETURN_SUCCESS != iFuncRet)
	{
		goto L_Err;
	}


L_Exit:
	/* free memory */
	FreeCommodityFavorablePriceInfoList(&pCommodityFavorablePriceInfoHead);
	FreeBuy2Get1FreeInfoList(&pBue2Get1FreeCommodityList);
	FreeCommodityInfoList(&pCommodityInfoList);

	return RETURN_SUCCESS;

L_Err:
	iFuncRet = RETURN_ERROR;
	goto L_Exit;
}

