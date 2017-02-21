//Programmer: DANIEL FUCHS
//Course: Algorithms, CS2500-A
//Date: 11/22/15
//
// - Communication Network Recovery Project
//Description: This program simulates various different recovery algorithms
//             to repair a sensor network. The sensor network used is based
//             off of the "Internet Topology Zoo" sensor network. This network's
//             layout information is provided in the Kdl.gml file.
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <limits.h>
#include <string.h>
#include <queue>
using namespace std;



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////GLOBAL CONSTANTS////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

const string FILENAME = "Kdl.gml"; //Name of input file
const int MRT = 100; //Maximum Repair Time
const int MLC = 10; //Maximum Link Capacity
const int SRCID = 52; //Source Node ID used for network
const int DSTID = 725; //Destination Node ID used for network
const int ITV = 50; //Intervals between flow recordings

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////CLASSES/////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//////////
///NODE///
//////////

class Node
{
  private:
    float xpos; //X-Coordinate
    float ypos; //Y-Coordinate
    int n_time; //Time to repair
    
  public:
    bool broken; //Status of Node

    //CONSTRUCTOR
    Node(float x, float y)
    {
      xpos = x;
      ypos = y;
      n_time = (rand()%MRT)+1;
      broken = false;
    }
    
    //ACCESSOR FUNCTIONS
    float getXP(){return xpos;}
    float getYP(){return ypos;}
    int getTIME(){return n_time;}
};

//////////
///LINK///
//////////

class Link
{
  private:
    int sid; //ID of first node attached to edge
    int eid; //ID of other node attached to edge
    int cap; //Capacity
    int l_time; //Time to repair
    float midx; //Midpoint's X-Coordinate
    float midy; //Midpoint's Y-Coordinate
    
  public:
    bool broken; //Status of Link
    bool connected; //True if both nodes are unbroken, false otherwise
  
    //CONSTRUCTOR
    Link(int s, int e, float x, float y)
    {
      sid = s;
      eid = e;
      cap = (rand()%MLC)+1;
      l_time = (rand()%MRT)+1;
      midx = x;
      midy = y;
      broken = false;
      connected = true;
    }
  
    //ACCESSOR FUNCTIONS
    int getSI(){return sid;}
    int getEI(){return eid;}
    int getCAP(){return cap;}
    int getTIME(){return l_time;}
    float getMX(){return midx;}
    float getMY(){return midy;}
    
};

/////////////
///NETWORK///
/////////////

class Network
{
  public:
    int node_count; //Number of Nodes
    int link_count; //Number of Links
    int nodes_broken; //Number of broken Nodes
    int links_broken; //Number of broken Links
    int** AM; //Adjacency Matrix for graphical representation of this Network
    vector<Node> v_node; //Stores all Nodes in the network
    vector<Link> v_link; //Stores all Links in the network
    int RRT; //Remaining Repair Time. Stores remaining amount of time until
    //a scheduled repair is completed
    bool LR; //Link Repair. 1 if a single Link is to be repaired
    bool NR; //Node Repair. 1 is a single Node is to be repaired
    bool SR; //Smart Repair. 1 if a smart repair is scheduled.
    int RI; //Repair Index. Stores index of item to be repaired.
    float b_x; //Barycenter's X-Coordinate
    float b_y; //Barycenter's Y-Coordinate
        
    //CONSTRUCTOR
    Network()
    {
      node_count = 0;
      link_count = 0;
      nodes_broken = 0;
      links_broken = 0;
      RRT = 0;
      LR = false;
      NR = false;
      SR = false;
      RI = -1;
      AM = NULL;
    }
    
    //COPY CONSTRUCTOR
    Network(Network & rhs)
    {
      node_count = rhs.node_count;
      link_count = rhs.link_count;
      nodes_broken = rhs.nodes_broken;
      links_broken = rhs.links_broken;
      AM = new int*[node_count];
      for (int i = 0; i < node_count; i++)
      {
        AM[i] = new int[node_count];
        for (int j = 0; j < node_count; j++)
        {
          AM[i][j] = rhs.AM[i][j];
        }
      }
      for (int i = 0; i < node_count; i++)
      {
        v_node.push_back(rhs.v_node[i]);
      }
      for (int i = 0; i < link_count; i++)
      {
        v_link.push_back(rhs.v_link[i]);
      }
      RRT = rhs.RRT;
      LR = rhs.LR;
      NR = rhs.NR;
      SR = rhs.SR;
      RI = rhs.RI;
      b_x = rhs.b_x;
      b_y = rhs.b_y;
    }
    
    //DESTRUCTOR
    ~Network()
    {
      for (int i = 0; i < node_count; i++)
      {
        delete []AM[i];
      }
      delete []AM;
    }
    
    //ACCESSOR FUNCTIONS
    int getNC(){return node_count;}
    int getLC(){return link_count;}
    int getNB(){return nodes_broken;}
    int getLB(){return links_broken;}

    //ASSESS DAMAGE
    //Description: Returns the time required to fix the entire network
    int assessDamage()
    {
      int duration = 0; //Stores time required to fix entire network
      for (int i = 0; i < node_count; i++)
      {
        if (v_node[i].broken)
        {
          duration += v_node[i].getTIME();
        }
      }
      for (int j = 0; j < link_count; j++)
      {
        if (v_link[j].broken)
        {
          duration += v_link[j].getTIME();
        }
      }
      return duration;
    }

    //REPAIR NODE
    //Description: Schedules a Node to be repaired
    void repairNode(const int index)
    {
      if (RRT != 0 && !v_node[index].broken)
      {
        cout << "Error, Illegal use of Repair()" << endl;
        return;
      }
      NR = true;
      RI = index;
      RRT = v_node[index].getTIME();
      return;
    }

    //REPAIR LINK
    //Description: Schedules a Link to be repaired
    void repairLink(const int index)
    {
      if (RRT != 0 || !v_link[index].broken)
      {
        cout << "Error, Illegal use of Repair()" << endl;
        return;
      }
      LR = true;
      RI = index;
      RRT = v_link[index].getTIME();
      return;
    }

    //SMART REPAIR TIME CALCULATOR
    //Description: Evaluates the required time to repair a Link and its two
    //related Nodes.
    int calcSRT(const int index)
    {
      int SRT = 0;
      if (v_link[index].broken)
      {
        SRT += v_link[index].getTIME();
      }
      if (v_node[v_link[index].getSI()].broken) //Link's first node is broken
      {
        SRT += v_node[v_link[index].getSI()].getTIME(); //Add more repair time
      }   
      if (v_node[v_link[index].getEI()].broken) //If other node is broken
      {
        SRT += v_node[v_link[index].getEI()].getTIME();
      } 
      return SRT;
    }

    //SMART REPAIR
    //Description: Schedules a Link to be repaired, and if either of the nodes
    //attached to that Link are broken, they are scheduled to be repaired too.
    void smartRepair(const int index)
    {
      if (RRT != 0 || v_link[index].connected)
      {
        cout << "Error, Illegal use of Repair()" << endl;
        return;
      }
      SR = true;
      RI = index;
      RRT = calcSRT(index);
      return;
    }
    
    //PROGRESS
    //Description: This function decreases the remaining time on any scheduled
    //repair. If RRT reaches zero, then the repair is completed, and the
    //repaired objects are marked as functional.
    void progress()
    {
      if (RRT > 0) //Repair is still being processed
      {
        cout << "Repair not yet completed." << endl;
      }
      else if (NR) //Node Repair was completed
      {
        NR = false;
        v_node[RI].broken = false;
        nodes_broken--;
        RI = -1;
        connect();
      }
      else if (LR) //Link Repair was completed
      {
        LR = false;
        v_link[RI].broken = false;
        links_broken--;
        RI = -1;
        connect();
      }
      else if (SR) //Smart Repair was completed
      {
        SR = false;
        int temp_s = v_link[RI].getSI();
        int temp_e = v_link[RI].getEI();
        if (v_node[temp_s].broken == true)
        {
          v_node[temp_s].broken = false;
          nodes_broken--;
        }
        if (v_node[temp_e].broken == true)
        {
          v_node[temp_e].broken = false;
          nodes_broken--;
        }
        if (v_link[RI].broken == true)
        {
          v_link[RI].broken = false;
          links_broken--;
        }
        RI = -1;
        connect();
      }
      else
      {
        cout << "Error. No repair was scheduled to be completed." << endl;
      }
      return;
    }
    
    //CONNECT
    //Description: This function updates all unconnected Links.
    void connect()
    {
      for (int k = 0; k < link_count; k++)
      {
        if (v_link[k].connected == false) //Link is marked as unconnected
        {
          if (!v_node[v_link[k].getSI()].broken &&
              !v_node[v_link[k].getEI()].broken &&
              !v_link[k].broken)
          //If both nodes are functional and the link is repaired
          {
            v_link[k].connected = true;
            AM[v_link[k].getSI()][v_link[k].getEI()] = v_link[k].getCAP();
            AM[v_link[k].getEI()][v_link[k].getSI()] = v_link[k].getCAP();
          }
        }
        else //Link is marked as connected
        {
          if (v_node[v_link[k].getSI()].broken ||
              v_node[v_link[k].getEI()].broken ||
              v_link[k].broken)
          //If either nodes is broken or the link itself is broken
          {
            v_link[k].connected = false;
            AM[v_link[k].getSI()][v_link[k].getEI()] = 0;
            AM[v_link[k].getEI()][v_link[k].getSI()] = 0;
          }
        }
      }
      return;
    }

    //RANDOM FAILING FUNCTION
    //Description: Based on the % passed, breaks sensors and links.
    void randomFail(const int percent)
    {
      int temp = 0;
      for (int i = 0; i < node_count; i++)
      {
        temp = (rand()%100)+1;
        if (temp <= percent)
        {
          v_node[i].broken = true;
          nodes_broken++;
        }
      }
      for (int i = 0; i < link_count; i++)
      {
        temp = (rand()%100)+1;
        if (temp <= percent)
        {
          v_link[i].broken = true;
          links_broken++;
        }
      }
      connect();
      return;
    }

    //GEOGRAPHIC FAILING FUNCTION
    //Description: Based on a circular region that contains all nodes, breaks
    //nodes within the passed % of the the regions's radius.
    void geoFail(const float percent)
    {
      float x = 0;
      float y = 0;
      float radq = 0; //Stores Radius Squared
      float rmax = -1;
  
      /*-----CALCULATING RADIUS-----*/
      for (int i = 0; i < node_count; i++)
      {
        x = v_node[i].getXP();
        y = v_node[i].getYP();
        radq = ((b_x - x)*(b_x - x) + (b_y - y)*(b_y - y));
        if (rmax < radq)
        {
          rmax = radq;
        }
      }
      /*-----BREAKING PHASE-----*/
      int temp = 0; //Stores distance from barycenter, squared
      for (int i = 0; i < node_count; i++)
      {
        x = v_node[i].getXP();
        y = v_node[i].getYP();
        temp = ((b_x - x)*(b_x - x) + (b_y - y)*(b_y - y));
        if (temp < ((percent/100)*rmax))
        {
          v_node[i].broken = true;
          nodes_broken++;
        }
      }
      for (int i = 0; i < link_count; i++)
      {
        x = v_link[i].getMX();
        y = v_link[i].getMY();
        temp = ((b_x - x)*(b_x - x) + (b_y - y)*(b_y - y));
        if (temp < ((percent/100)*rmax))
        {
          v_link[i].broken = true;
          links_broken++;
        }
      }
      connect();
      return;
    }

};
  
    

///////////////////////////////////////////////////////////////////////////////
//////////////////////////////GENERAL FUNCTIONS////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//BREADTH FIRST SEARCH
//Description: Special version of BFS used in conjuction with the
//following Max-Flow-Calculating Algorithm.
bool bfs(int** RG, int s, int t, int* parent, int nodes)
{
  queue<int> Q; //Storage queue
  Q.push(s); //Push starting node into queue
  bool* visited = new bool[nodes]; //Stores "visited" status on all nodes
  for (int i = 0; i < nodes; i++) //Set all visited[] -> 0
  {
    visited[i] = false;
  }
  visited[s] = true;
  parent[s] = -1; //"NIL"
  while (Q.empty() == false) //While Q is not empty
  {
    int u = Q.front();
    Q.pop();
    for (int v = 0; v < nodes; v++)
    {
      if (!visited[v] && RG[u][v] > 0)
      {
        Q.push(v);
        parent[v] = u;
        visited[v] = true;
      }
    }
  }
  bool storage = visited[t];
  delete []visited;
  return storage;
}

//FORD FULKERSON'S MAX FLOW CALCULATOR
//Description: Used to calculate Max Flow. Based off of the Ford
//Fulkerson Algorithm; this particular implementation is known as the
//Edmonds-Karp Algorithm.
int calcMaxFlow(int** graph, int s, int t, int nodes)
{
  int max_flow = 0; //Max calculated flow
  int u = 0;
  int v = 0;

  int** RG = new int*[nodes]; //Stores Residual Graph
  for (int i = 0; i < nodes; i++) //Create 2D Array
  {
    RG[i] = new int[nodes];
  }
  for (u = 0; u < nodes; u++) //Copy original graph into RG
  {
    for (v = 0; v < nodes; v++)
    {
      RG[u][v] = graph[u][v];
    }
  }
  int* parent = new int[nodes]; //Array to store parent nodes

  /*-----MAX FLOW CALCULATION-----*/
  while (bfs(RG, s, t, parent, nodes))
  {
    int path_flow = INT_MAX;
    for (v = t; v != s; v = parent[v])
    {
      u = parent[v];
      path_flow = min(path_flow, RG[u][v]);
    }
    for (v = t; v != s; v = parent[v])
    {
      u = parent[v];
      RG[u][v] -= path_flow;
      RG[v][u] += path_flow;
    }
    max_flow += path_flow;
  }

  /*-----MEMORY CLEANUP-----*/
  for (int i = 0; i < nodes; i++)
  {
    delete []RG[i];
  }
  delete []RG;
  delete []parent;
  return max_flow;
}

//PARSING FUNCTION
//Description: Reads from the input file, and creates the actual network.
//Must contain at least two nodes and one edge.
void parseGML(Network & comm_net)
{
  string storage = "empty"; //Storage string
  Node* temp_node; //Pointer to Node
  Link* temp_link; //Pointer to Link
  bool lcv = true; //Boolean to control loops
  ifstream fin;
  fin.open(FILENAME.c_str());
  /*-----SEARCH FOR START OF NODE LIST-----*/
  while(storage != "node") //When "node" is read, the node list has begun
  {
    fin >> storage;
  }
  /*-----BEGIN FORMING NODES-----*/
  float x = 0; //Stores Latitude
  float y = 0; //Stores Longitude
  while(storage != "edge") //When "edge is read", the edge list has begun
  {
    while(storage != "Longitude" && lcv)
    {
      fin >> storage;
      if (storage == "hyperedge")
      {
        lcv = false;
      }
    }
    if(lcv)
    {
      fin >> y;
    }
    while(storage != "Latitude" && lcv)
    {
      fin >> storage;
    }
    if (storage != "Latitude") //Hyperedges do not have coordinates.
    { //So, a generic coordinate near the center of the system is used.
      x = 40;
      y = -90;
    }
    else
    {
      fin >> x;
    }
    temp_node = new Node(x,y);
    comm_net.v_node.push_back(*temp_node);
    comm_net.node_count++;
    delete temp_node;
    temp_node = NULL;
    lcv = true;
    while(storage != "node" && storage != "edge")
    {
      fin >> storage;
    }
  }

  //*-----ADJACENCY MATRIX CREATION-----*/
  comm_net.AM = new int*[comm_net.node_count];
  for (int i = 0; i < comm_net.node_count; i++)
  {
    comm_net.AM[i] = new int[comm_net.node_count];
    for (int j = 0; j < comm_net.node_count; j++)
    {
      comm_net.AM[i][j] = 0;
    }
  }

  /*-----BEGIN FORMING LINKS-----*/
  int s = 0; //Stores Link's first node
  int e = 0; //Stores Link's other node
  x = 0;
  y = 0;  
  while(lcv)
  {
    while(storage != "edge")
    {
      fin >> storage;
    }
    while(storage != "source")
    {
      fin >> storage;
    }
    fin >> s;
    while(storage != "target")
    {
      fin >> storage;
    }
    fin >> e;

    /*-----CALCULATE X AND Y FOR LINK-----*/
    x = (comm_net.v_node[s].getXP() + comm_net.v_node[e].getXP())/2;
    y = (comm_net.v_node[s].getYP() + comm_net.v_node[e].getYP())/2;
    temp_link = new Link(s,e,x,y); //Start, End, X-Coord, Y-Coord.
    comm_net.v_link.push_back(*temp_link);
    comm_net.link_count++;
    comm_net.AM[s][e] = temp_link->getCAP();
    comm_net.AM[e][s] = temp_link->getCAP();
    delete temp_link;
    temp_link = NULL;
    while(storage != "]")
    {
      fin >> storage;
    }
    fin >> storage;
    if (storage == "]")
    {
      lcv = false;
    }
  }

  /*-----CALCULATE BARYCENTER-----*/
  float bary_x = 0;
  float bary_y = 0;
  for (int i = 0; i < comm_net.getNC(); i++)
  {
    bary_x += comm_net.v_node[i].getXP();
    bary_y += comm_net.v_node[i].getYP();
  }
  comm_net.b_x = bary_x / comm_net.getNC();
  comm_net.b_y = bary_y / comm_net.getNC();
  fin.close();
  return;
}

//RANDOM REPAIR
//Description: Implementation of Random Algorithm. Schedules a random broken
//component for repairs if no other components are currently being repaired.
void randomRepair(Network & comm_net)
{
  if (comm_net.RRT != 0)
  {
    return;
  }
  else //The network is not currently repairing anything else
  {
    if (comm_net.LR || comm_net.NR || comm_net.SR)
    {
      comm_net.progress(); //Completes scheduled repair
    }
    int repair_count = -1; //Stores amount of repairs to be made
    repair_count = comm_net.getNB() + comm_net.getLB();
    int midpoint = comm_net.getNB();
    if (repair_count == 0) //No repairs are necessary
    {
      return;
    }
    int selection = (rand()%repair_count)+1;
    bool searching = true; //Controls loops that search for broken component
    if (selection <= midpoint) //A node will be repaired
    {
      while(searching)
      {
        selection = (rand()%comm_net.getNC());
        if (comm_net.v_node[selection].broken == true)
        {
          comm_net.repairNode(selection); //Schedule Node Repair
          searching = false;
        }
      }
    }
    else //A link will be repaired
    {
      while(searching)
      {
        selection = (rand()%comm_net.getLC());
        if (comm_net.v_link[selection].broken == true)
        {
          comm_net.repairLink(selection); //Schedule Link Repair
          searching = false;
        }
      }
    }
    return;
  }
}

//ALGORITHM REPAIR
//Description: Implementation of the Algorithm designed in the report.
//The "ERV" for some Link is the ratio formed by the capacity of the link
//divided by the time to repair the link and the two nodes tied to it.
//If the link or either of the nodes are already functional, then their
//repair time is not added into this ratio.
void algorithmicRepair(Network & comm_net)
{
  if (comm_net.RRT != 0)
  {
    return;
  }
  else //The network is not currently repairing anything else
  {
    if (comm_net.LR || comm_net.NR || comm_net.SR)
    {
      comm_net.progress(); //Completes scheduled repair
    }
    int repair_count = comm_net.getNB() + comm_net.getLB();
    if (repair_count == 0) //No repairs are necessary
    {
      return;
    }
    float temp_ERV = 0;
    float max_ERV = -1;
    float duration = -1;
    int ERV_index = -1;
    for (int l = 0; l < comm_net.getLC(); l++)
    {
      if (comm_net.v_link[l].connected == false)
      {
        duration = comm_net.calcSRT(l);
        temp_ERV = (static_cast<float>(comm_net.v_link[l].getCAP())/duration);
        if (temp_ERV > max_ERV)
        {
          max_ERV = temp_ERV;
          ERV_index = l;
        }
      }
    }
    comm_net.smartRepair(ERV_index);
    return;
  }
}



///////////////////////////////////////////////////////////////////////////////
/////////////////////////////////MAIN PROGRAM//////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int main()
{
  srand(time(NULL));
  Network randNet;
  parseGML(randNet);
  int max_flow = calcMaxFlow(randNet.AM, SRCID, DSTID, randNet.getNC());
  cout << "Initial Flow: " << max_flow << endl << endl;
  
  /*-----MODE SELECTION-----*/
  bool mode = true; //Used to determine geographical or random failure
  cout << "Select Mode: (0) Geographical Failure, (1) Random Failure" << endl;
  cout << "Mode: ";
  cin >> mode;
  int PROB = 0; //Stores Probability
  if (mode) //Random Failure Selected
  {
    cout << endl << "Enter Percent Failure Rate: ";
    cout << endl << "Percent: ";
    cin >> PROB;
    randNet.randomFail(PROB);
  }
  else //Geographical Failure Selected
  {
    cout << endl << "Enter Percent of Diameter Destroyed: ";
    cout << endl << "Percent: ";
    cin >> PROB;
    randNet.geoFail(PROB);
  }
  Network algNet(randNet); //Creates Duplicate Network
  cout << endl;
  
  /*-----EXPERIMENTS-----*/
  int clock = 0; //Stores time passed
  int rec_c = 0; //Stores number of recorded values
  int est_t = randNet.assessDamage(); //Stores time to recover full network
  int* RNflow = new int[ITV+1]; //Stores Random Algorithm Flow Measurements
  int* ANflow = new int[ITV+1]; //Stores Greedy Algorithm flow Measurements
  for (int i = 0; i < ITV+1; i++) //Fills out initial flow values
  {
    RNflow[i] = max_flow; //Default flow is optimal
    ANflow[i] = max_flow; //Default flow is optimal
  }

  //RANDOM ALGORITHM TESTING PHASE
  while(randNet.assessDamage() != 0)
  {
    randomRepair(randNet);
    if (randNet.RRT > 0) //If there are still repairs left
    {
      randNet.RRT--;
    }
    if (clock == (rec_c*(est_t/(ITV)))) //If an interval was reached
    {
      RNflow[rec_c] = calcMaxFlow(randNet.AM, SRCID, DSTID, randNet.getNC());
      //Record data values
      rec_c++;
    }    
    if (randNet.assessDamage() == 0) //Stores last values
    {
      RNflow[ITV] = calcMaxFlow(randNet.AM, SRCID, DSTID, randNet.getNC());
    }
    clock++;
  }

  //GREEDY ALGORITHM TESTING PHASE
  clock = 0; //Reset
  rec_c = 0; //Reset
  while(algNet.assessDamage() != 0)
  {
    algorithmicRepair(algNet);
    if (algNet.RRT > 0) //If there are still repairs left
    {
      algNet.RRT--;
    }
    if (clock == (rec_c*(est_t/(ITV)))) //If an interval was reached
    {
      ANflow[rec_c] = calcMaxFlow(algNet.AM, SRCID, DSTID, algNet.getNC());
      //Record data values
      rec_c++;
    }
    if (algNet.assessDamage() == 0) //Stores last values
    {
      ANflow[ITV] = calcMaxFlow(algNet.AM, SRCID, DSTID, algNet.getNC());
    }
    clock++;
  }
  
  /*-----OUTPUT-----*/
  float avgR = 0;
  float avgA = 0;
  cout << "Flow Analysis: " << endl;
  cout << "(R,A)" << endl;
  for (int i = 0; i <= ITV; i++)
  {
    cout << "(" << RNflow[i] << "," << ANflow[i] << ")" << endl;
    avgR += RNflow[i];
    avgA += ANflow[i];
  }
  avgR /= (ITV+1);
  avgA /= (ITV+1);
  cout << "Random Algorithm's Average Flow: " << avgR << endl;
  cout << "Greedy Algorithm's Average Flow: " << avgA << endl;

  /*-----DATA CLEANUP-----*/
  delete []ANflow;
  delete []RNflow;
  return 0;
}



