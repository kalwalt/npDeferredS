
#include "Chunk.h"



Chunk::Chunk(){
	
	width =20;
	height=20 ;
	posX =1000000;
	posY =10000;
	isReady =false;
	terrain = new npMesh();
	cDivX =1;
	cDivY =1;
	currentDetailMaterial =NULL;
	heightBody =NULL;
}


void Chunk::update()
{
	float worldX = posX *width;
	float worldY = posY *height;



	float cStepY  = (float)height/cDivY;
	float cStepX  =(float) width/cDivX;

	for(int y=-1;y<cDivY +2 ;y++)
	{
		int pPosY = y* cStepY;
		for(int x=-1;x<cDivX+2;x++)
		{
			int pPosX = x* cStepX ;
		
			
			TerrainVertex vertex;
			vertex.positionT.x = (float) pPosX  +worldX;
			vertex.positionT.z =  (float) pPosY  +worldY;
			vertex.positionT.y = terrainFunctions-> getHeightForPos( 	vertex.positionT.x,vertex.positionT.z);
			 terrainFunctions->getNormalforVertex(vertex);

			 terrainVertices.push_back(vertex);
		}
	}

	///blur
	for(int y=0;y<cDivY +1 ;y++)
	{
		
		for(int x=0;x<cDivX+1;x++)
		{
		
			TerrainVertex * vertex= getVertexForXY(x,y);
			vertex->position.set(0,0,0);
			vertex->normal.set(0,0,0);
			for(int bx=-1;bx<2;bx++)
			{
			
				for(int by=-1;by<2;by++)
				{
					TerrainVertex * vertexT = getVertexForXY(x+bx,y+by);
					vertex->position+= vertexT->positionT; 
					vertex->normal+= vertexT->normalT; 
				}
			}
			vertex->normal.normalize();
			vertex->position/=9;
			vertex->hil =vertex->normal.dot(ofVec3f(0,1,0));
			vertex->uv.x = (float)x/(cDivX+1);
			vertex->uv.y = (float)y/(cDivY+1);

			//cout <<vertex->hil;

			
		}
	}




	if ( terrain->numVertices == -1)
	{

		buildFirst();
		return;
	} 
	



	npMesh * mesh = terrain;
	

	


	int vertcount =0;
	for(int y=0;y<cDivY +1 ;y++)
	{
		
		for(int x=0;x<cDivX+1;x++)
		{
			
	
			TerrainVertex * vertex= getVertexForXY(x,y);

			mesh->vertices[vertcount++] = vertex->position.x;
            mesh->vertices[vertcount++] = vertex->position.y;
            mesh->vertices[vertcount++] = vertex->position.z;
            
         
            
            mesh->vertices[vertcount++] = vertex->normal.x;
            mesh->vertices[vertcount++] =  vertex->normal.y;
            mesh->vertices[vertcount++] = vertex->normal.z;
	

			vertcount+=4;//uv*2
			//vertcount++;





		}
		
	}
	
	  glBindBuffer(GL_ARRAY_BUFFER, mesh->vertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER,0, mesh->numVertices*mesh->stride* sizeof(float), mesh->vertices);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);


	///

	makeTerrainObjects();




	isReady =true;
if	(detailLevel==1)
	{
		
		addPhysicsHeightField();
	
	}

}

void Chunk::makeTerrainObjects()
{
	

	terrainFunctions->startNewObjectsForChunk(this);

	for(int y=0;y<cDivY +1 ;y++)
	{
	
		for(int x=0;x<cDivX+1;x++)
		{
		
			terrainFunctions->getObjectsForVertex (getVertexForXY( x,  y),this);
    
		}
	
	}
	terrainFunctions->stopNewObjectsForChunk(this);
	detail1Objects[1].makeOneMesh();
detail2Objects[1].makeOneMesh();
//detail2Objects[4].makeOneMesh();

/*detail1Objects[3].makeOneMesh();
detail2Objects[3].makeOneMesh();


detail2Objects[4].makeOneMesh();
					
detail3Objects[0].makeOneMesh();*/
}
TerrainVertex * Chunk::getVertexForXY(int x, int y)
{

	int pos = (x +1) +(y+1)*(cDivY +3);
	//cout << "pos "<<pos << "  ->"<< x << ","<<y << endl;

	return  &terrainVertices[pos];



}

void Chunk::clearCurrent()
{

		isReady =false;
		terrainVertices.clear();

		
		detail1Objects.clear();
		detail2Objects.clear();
		detail3Objects.clear();


		detail1ObjectsTrans.clear();
		detail2ObjectsTrans.clear();
		detail3ObjectsTrans.clear();

		pLights.clear();
}




void Chunk::setPos(int posCenterX ,int posCenterY)
{

	
	if (posCenterX-numChunksW2> posX)
	{
		
		 posX = posCenterX +numChunksW2-1;
		 clearCurrent();
	
	
	}else if (posCenterX+numChunksW2<posX)
	{
		// xRechts
		posX = posCenterX -numChunksW2+1;
		clearCurrent();
	}

	if (posCenterY-numChunksW2> posY)
	{
	
		 posY = posCenterY +numChunksW2-1;
		clearCurrent();
	
	}else if (posCenterY+numChunksW2<posY)
	{
		posY = posCenterY -numChunksW2+1;
		clearCurrent();
	}
	

	center.set(posX*width+width/2,0,posY*height+height/2);

}










void Chunk::calculateDetail(const ofVec3f &camPos,const ofVec3f &lookDir)
{



	float dist = center.distance(camPos);
	ofVec3f dirChunk =  center-camPos; 
	 dirChunk.normalize();
	
	 float dot = lookDir.dot( dirChunk);
	 updatePrio = 1/dist ;
	 if( dot < 0)updatePrio=(1.0 -updatePrio) *-1; 
	 // 29  = width*wisth* 2 <-sqrt
	if (dist< 362*0.6)updatePrio =2;
	if (isReady ==false)
	{
	setDetailLevel(0);
	
	return;
	}
	
	
	if ( dist<362*0.6)
	{
	setDetailLevel(1);
	return;
	}
	else if( dist< width*4)
	{

		if (dot>0.3)
		{
			//
			
			setDetailLevel(2);
			//
			
			return;
		}
		setDetailLevel(0);
		
	}
	else if (dot>0.3)
	{	
			setDetailLevel(3);
			return;
	}
	setDetailLevel(0);
	
}
void Chunk::setDetailLevel(int _detailLevel)
{
	if (detailLevel==1 && _detailLevel!=1)
	{
	// free texture
		if (currentDetailMaterial) currentDetailMaterial->isUsed =false;
		removePhysicsHeightField();
	}
	else if(detailLevel!=1 && _detailLevel==1)
	{
		texureHandler->getChunkMaterial(this);
		if (isReady)addPhysicsHeightField();
	
	}
	detailLevel = _detailLevel;

}







void  Chunk::buildFirst()
{
	float totalWorld = (numChunksW2*2*width);
	
	float halfWorld = totalWorld/2; 
	float worldX = posX *width;
	float worldY = posY *height;

	float cStepY  = (float)height/cDivY;
	float cStepX  =(float) width/cDivX;

		float uvXStart =(float)((posXStart) +numChunksW2)/(numChunksW2*2);
	float uvYStart =(float)((posYStart)  +numChunksW2)/(numChunksW2*2);
	float uvXStep =1.0f/(numChunksW2*2*cDivX );
	float uvYStep =1.0f/(numChunksW2*2*cDivY);

	vpX =uvXStart*2048*2;
	vpY =uvYStart*2048*2;
	vpW = 128;
	vpH = 128;
	
	
	
	terrain =new npMesh();
	npMesh * mesh = terrain;
	
	mesh->stride = 10;
	mesh->numVertices =(cDivX+1) *(cDivY+1); 
	
	mesh->vertices =new float [mesh->numVertices*	mesh->stride ];

	mesh->numIndices  = (cDivX )* (cDivY ) *6 ;
	mesh->indices =new unsigned int[mesh->numIndices ];

		mesh->numIndices2  = (cDivX/4 )* (cDivY/4 ) *6 ;
	mesh->indices2 =new unsigned int[mesh->numIndices ];


	int vertcount =0;
	
	for(int y=0;y<cDivY +1 ;y++)
	{
	//	int pPosY = y* cStepY;
		for(int x=0;x<cDivX+1;x++)
		{
		
			TerrainVertex * vertex= getVertexForXY(x,y);

			mesh->vertices[vertcount++] = vertex->position.x;
            mesh->vertices[vertcount++] = vertex->position.y;
            mesh->vertices[vertcount++] = vertex->position.z;
            
         
            
            mesh->vertices[vertcount++] = vertex->normal.x;
            mesh->vertices[vertcount++] =  vertex->normal.y;
            mesh->vertices[vertcount++] = vertex->normal.z;



		
			mesh->vertices[vertcount++] =uvXStart+(x)*uvXStep ;
            mesh->vertices[vertcount++] =uvYStart+(y)*uvYStep;
	
				mesh->vertices[vertcount++] =(float) x/cDivX ;
            mesh->vertices[vertcount++] =(float) y/cDivY ;





		}
		
	}
	int indexCount = 0;
	for(int y=0;y<cDivY ;y++)
	{
	
		for(int x=0;x< cDivX;x++)
		{
			mesh->indices[indexCount++] = x+(y* (cDivX+1));
			
			mesh->indices[indexCount++] = x+((y+1)* (cDivX+1));
			mesh->indices[indexCount++] = x+1+(y* (cDivX+1));


			
			mesh->indices[indexCount++] = x+1+((y+1)* (cDivX+1));
			mesh->indices[indexCount++] = x+1+(y* (cDivX+1));
			mesh->indices[indexCount++] = x+((y+1)* (cDivX+1));
		}
	
	}
	indexCount = 0;
	for(int y=0;y<cDivY ;y+=4)
	{
	
		for(int x=0;x< cDivX;x+=4)
		{
			mesh->indices2[indexCount++] = x+(y* (cDivX+1));
			
			mesh->indices2[indexCount++] = x+((y+4)* (cDivX+1));
			mesh->indices2[indexCount++] = x+4+(y* (cDivX+1));


			
			mesh->indices2[indexCount++] = x+4+((y+4)* (cDivX+1));
			mesh->indices2[indexCount++] = x+4+(y* (cDivX+1));
			mesh->indices2[indexCount++] = x+((y+4)* (cDivX+1));
		}
	
	}

	mesh->createBuffers(GL_DYNAMIC_DRAW);
	mesh->objectMatrix.makeIdentityMatrix();
	mesh->normalMatrix.makeIdentityMatrix();


		makeTerrainObjects();
	isReady =true;
}


void  Chunk::removePhysicsHeightField()
{


}
void  Chunk::addPhysicsHeightField()
{
	cout << "addPhysics" <<terrainVertices.size()<<endl;
	if (heightBody)return;
	int numHeights  = terrainVertices.size();
	float *heightfieldData  = new float [numHeights];
	float maxHeight =-100000.0f;
	float minHeight =10000.0f;

	float _height;

	int count =0;
		for(int y=0;y<cDivY +1 ;y++)
	{
		
		for(int x=0;x<cDivX+1;x++)
		{
		
			TerrainVertex * vertex= getVertexForXY(x,y);
			_height=vertex->position.y;
				heightfieldData[count++] =_height;
	
		if (_height>maxHeight)maxHeight = _height;
		if (_height<minHeight)minHeight =_height;
		}
		
	}

/*
	for(int i=0;i<numHeights;i++)
	{
		_height = terrainVertices[i].position.y;
		
		heightfieldData[i] =_height;
	
		if (height>maxHeight)maxHeight = _height;
		if (height<minHeight)minHeight =_height;
	
	
	}*/
	//maxHeight  =400;
//minHeight  =0;
	cout << maxHeight <<" "<<minHeight<<" "<<(float)height/cDivY<<endl;
	float  dif = ( maxHeight -minHeight)/2.0f;
	heightShape  = new btHeightfieldTerrainShape(cDivX+1,cDivY+1,heightfieldData,1,minHeight,maxHeight,1,  PHY_FLOAT, false);
	heightShape->setLocalScaling(btVector3(4,1, 4));
	btTransform tr;
	tr.setIdentity();
	tr.setOrigin(btVector3(center.x, minHeight+dif, center.z));
	cout<<"center: " <<center.x<<" "<< center.z<<endl;
	btDefaultMotionState *m_pMotionState = new btDefaultMotionState(tr);

	heightBody = new btRigidBody(0 , m_pMotionState,heightShape );
	PhysicsHandler::getInstance()->dynamicsWorld->addRigidBody(heightBody );
}