
#include "bits_stdc++.h"
#include<mutex>
#include<unistd.h>   

using namespace std;

std::mutex mtx;

enum class Dir{
    UP, DOWN, STAY
};

class ElevatorRequest{

    public:
    int servingElevator;
    Dir dir;
    char name;
    int src_floor, dest_floor, o_dist;
    ElevatorRequest(Dir pdir, int psrc_floor, int pdest_floor, char nm): dir(pdir), 
    src_floor(psrc_floor), dest_floor(pdest_floor), o_dist(-1),name(nm), servingElevator(-1)
    {}
    string printRequest(){
        return "from "+ to_string(src_floor) +" to "+to_string(dest_floor)+"\n";
    }
};


class Elevator{
    // Status -> location, direction
    int elevator_num, min_floor, max_floor;
    vector<ElevatorRequest*> pickupRequests;
    vector<ElevatorRequest*> dropRequests;
    set<int> disabled_floors;
    // curr_dir = 1 going up
    // curr_dir = 2 down
    // curr_dir = 0 stay

    void serve(){

        vector<ElevatorRequest*> new_pickupRequests;
        vector<ElevatorRequest*> new_dropRequests;

        for(auto dropRequest:dropRequests){
            if(dropRequest->dest_floor == curr_floor){
                mtx.lock();
                cout<<"elevator-"<<elevator_num<<" drops "<<dropRequest->name<<" on floor "<< curr_floor<<" from "<<dropRequest->src_floor<<"\n";
                mtx.unlock();
            }
            else{
                new_dropRequests.push_back(dropRequest);
            }
        }
        for(auto pickupRequest:pickupRequests){
            if(pickupRequest->src_floor == curr_floor && pickupRequest->dir == curr_dir){
                mtx.lock();
                cout<<"elevator-"<<elevator_num<<" picks up "<<pickupRequest->name<<" on floor "<< curr_floor<<" towards "<<pickupRequest->dest_floor<<"\n";
                mtx.unlock();
                new_dropRequests.push_back(pickupRequest);
            }
            else{
                new_pickupRequests.push_back(pickupRequest);
            }
        }
        // mtx.lock();
        dropRequests = new_dropRequests;
        pickupRequests = new_pickupRequests;
        // mtx.unlock();

    }

    void move_up(){
        curr_dir = Dir::UP;
        curr_floor++;
        if(curr_floor>=max_floor){
            curr_dir = Dir::DOWN;
            curr_floor = max_floor;
        }
        serve();
    }
    void move_down(){
        curr_dir = Dir::DOWN;
        curr_floor--;
        if(curr_floor<=min_floor){
            curr_dir = Dir::UP;
            curr_floor = min_floor;
        }
        serve();
    }

    public:
    int curr_floor;
    Dir curr_dir;
    Elevator(int elevator_num_, int floor_loc_) : pickupRequests{}, dropRequests{}
    {
        elevator_num = elevator_num_;
        curr_floor = floor_loc_;
        curr_dir = Dir::STAY;
    }
    int getElevatorNum(){
        return elevator_num;
    }
    void setFloors(int minfloor, int maxfloor){
        min_floor = minfloor;
        max_floor = maxfloor;
    }

    void addPickup(ElevatorRequest* ereq){
        mtx.lock();
        pickupRequests.push_back(ereq);
        mtx.unlock();
        ereq->servingElevator = elevator_num;
    }
    void moveNext(){
        int just_up=INT_MAX,just_down=INT_MAX;
        // ElevatorRequest* just_up_req, *just_down_req, *pickupRequest;
        int up_i,down_i;

        if(dropRequests.size()>0){
            for(auto dropRequest:dropRequests){
                // 
                if(dropRequest->dest_floor > curr_floor){
                    if(just_up > (dropRequest->dest_floor - curr_floor)){
                        just_up = (dropRequest->dest_floor - curr_floor);
                    }
                }
                else if(dropRequest->dest_floor < curr_floor){
                    if(just_down > (curr_floor - dropRequest->dest_floor)){
                        just_down = (curr_floor - dropRequest->dest_floor);
                    }
                }
            }
        }
        else{
            for(ElevatorRequest* pickupRequest:pickupRequests){
                // 
                if(pickupRequest->src_floor > curr_floor){
                    if(just_up > (pickupRequest->src_floor - curr_floor)){
                        just_up = (pickupRequest->src_floor - curr_floor);
                    }
                }
                else if(pickupRequest->src_floor < curr_floor){
                    if(just_down > (curr_floor - pickupRequest->src_floor)){
                        just_down = (curr_floor - pickupRequest->src_floor);
                    }
                }
                else{
                    curr_dir = pickupRequest->dir;
                    serve();
                    break;
                }
            }
        }
        if(curr_dir==Dir::STAY){
            if(just_up<just_down){
                move_up();
            }
            else if(just_up> just_down){
                move_down();
            }
        }
        else if(curr_dir==Dir::UP){
            if(just_up<INT_MAX){
                move_up();
            }
            else if(just_down<INT_MAX){
                move_down();
            }
        }
        else if(curr_dir==Dir::DOWN){
            if(just_down<INT_MAX){
                move_down();
            }
            else if(just_up<INT_MAX){
                move_up();
            }
        }
    }


    void disableFloor(int floorId){
        disabled_floors.insert(floorId);
    }

    void enableFloor(int floorId){
        if(disabled_floors.find(floorId)!=disabled_floors.end()){
            disabled_floors.erase(floorId);
        }
    }

    bool checkIfFloorsDisabled(int src_floor, int dst_floor){
        if(disabled_floors.find(src_floor)==disabled_floors.end() && disabled_floors.find(dst_floor)==disabled_floors.end()){
            return true;
        }
        return false;
    }

};

class Floor{
    // request status up and/or down
    int floor_num;
    public:
    Floor(int floor_num_){
        floor_num = floor_num_;
    }
    int getFloorNum(){
        return floor_num;
    }
};


class ElevatorDB{
    public:
    map<int, Elevator*> elevators_map;
};

class FloorDB{
    public:
    map<int, Floor*> floors_map;
};


class ElevatorService{
    // object entries has relation
    // Floors, Elevators
    ElevatorDB* elevatorDB;
    FloorDB* floorDB;
    int maxfloor, minfloor;
    bool isRunning;
    static queue<ElevatorRequest*> eRequests;
    static ElevatorService* eServiceIns;
    ElevatorService(vector<Elevator*> elevators, vector<Floor*> floors){
        maxfloor=INT_MIN;
        minfloor=INT_MAX;
        isRunning = true;
        floorDB = new FloorDB();
        elevatorDB = new ElevatorDB();
        for(Floor* floor:floors){
            floorDB->floors_map[floor->getFloorNum()] = floor;
            maxfloor = max(maxfloor, floor->getFloorNum());
            minfloor = min(minfloor, floor->getFloorNum());
        }

        for(Elevator* elevator:elevators){
            elevatorDB->elevators_map[elevator->getElevatorNum()] = elevator;
            elevator->setFloors(minfloor, maxfloor);
        }
    }
    public:
    static ElevatorService* getInstance(vector<Floor*> floors, vector<Elevator*> elevators){

        if(eServiceIns==NULL){
            eServiceIns = new ElevatorService(elevators,floors);
        }
        else{
            mtx.lock();
            cout<<"There is already a ElevatorService instance, You may edit it"<<endl;
            mtx.unlock();
        }
        return eServiceIns;
    }
    static ElevatorService* findInstance(){
        return eServiceIns;
    }
    // Add, remove floors
    // Add, remove elevators

    string scheduleTrip(int srcFloor, int destFloor,char name){
        if(srcFloor==destFloor){
            return "Cannot accept same floor movement";
        }
        ElevatorRequest* ereq;
        if(srcFloor< destFloor){
            // UP
            ereq = new ElevatorRequest(Dir::UP, srcFloor, destFloor,name);
        }
        else{
            //DOWN
            ereq = new ElevatorRequest(Dir::DOWN, srcFloor, destFloor,name);
        }
        mtx.lock();
        eRequests.push(ereq);
        mtx.unlock();
        while(ereq->servingElevator ==-1){

        }
        if(ereq->servingElevator==-2){
            return "No elevator is ready to pick";
        }
        return "elevator-"+to_string(ereq->servingElevator)+" will be picking "+ereq->name;
    }

    void assignElevator(ElevatorRequest* req){

        Elevator* elevator, *pickEl=NULL;
        int o_dist,fin_odist=INT_MAX;
        for(auto elevator_pair:elevatorDB->elevators_map){
            elevator = elevator_pair.second;
            if(elevator->curr_dir == req->dir || elevator->curr_dir==Dir::STAY){
                if(req->dir==Dir::UP){
                    o_dist = (req->src_floor > elevator->curr_floor)? (req->src_floor - elevator->curr_floor):
                                            2*maxfloor-(elevator->curr_floor)-(req->src_floor);
                }
                else{
                    o_dist = (req->src_floor < elevator->curr_floor)?(elevator->curr_floor - req->src_floor):
                                            (elevator->curr_floor)+(req->src_floor)-2*minfloor;
                }
            }
            else{
                if(req->dir==Dir::UP){
                    o_dist = (elevator->curr_floor)+(req->src_floor)-2*minfloor;
                }
                else{
                    o_dist = 2*maxfloor-(elevator->curr_floor)-(req->src_floor);
                }
            }
            if (o_dist<fin_odist && elevator->checkIfFloorsDisabled(req->src_floor, req->dest_floor)){
                pickEl = elevator;
                fin_odist = o_dist;
            }

        }
        if(pickEl==NULL){
            req->servingElevator = -2;
            return;
        }
        req->o_dist = fin_odist;
        pickEl->addPickup(req);
        
    }

    static void serviceRequests(){
        ElevatorService* eService = ElevatorService::findInstance();
        while(true){
            if(!eRequests.empty()){
                ElevatorRequest* req = eRequests.front();
                mtx.lock();
                eRequests.pop();
                mtx.unlock();
                if(req==NULL){
                    continue;
                }

                //cout<<req->printRequest();
                eService->assignElevator(req);
            }
        }
    }
    void moveSystem(){
        Elevator* elevator;
        for(auto elevator_pair:elevatorDB->elevators_map){
            elevator = elevator_pair.second;

            elevator->moveNext();
        }
        
    }

    void runningElevators(){
        isRunning = true;
        while(isRunning){
            moveSystem();
            sleep(1);
        }
    }

    void removeAElevator(int elevatorId){

        Elevator* elevator=elevatorDB->elevators_map[elevatorId];
        elevatorDB->elevators_map.erase(elevatorId);
        delete elevator;
    }

    void addAElevator(int elevatorId){

        elevatorDB->elevators_map[elevatorId] = new Elevator(elevatorId,1);
        elevatorDB->elevators_map[elevatorId]->setFloors(minfloor,maxfloor);
    }

    void disableAFloor(int elevatorId, int floorId){
        elevatorDB->elevators_map[elevatorId]->disableFloor(floorId);
    }

    void enableAFloor(int elevatorId, int floorId){
        elevatorDB->elevators_map[elevatorId]->enableFloor(floorId);
    }

    static void startRunningElevators(){
        ElevatorService* eService = ElevatorService::findInstance();
        eService->runningElevators();
    }
};

ElevatorService* ElevatorService::eServiceIns = NULL;
queue<ElevatorRequest*> ElevatorService::eRequests = queue<ElevatorRequest*>();


// Like to request elevator using any mobile device
class Client{
    // requests (from floor and to floor)
    // Status in a elevator, on a floor outside
    ElevatorService* elevatorService;
    public:
    Client(){
        elevatorService = ElevatorService::findInstance();
    }
    // Request 
    void requestElevator(int src_floor, int dest_floor,char name){
        mtx.lock();
        cout<<"scheduling "<<src_floor<<" -> "<<dest_floor<<" for "<<name<<"\n";
        mtx.unlock();
        string schedule_msg = elevatorService->scheduleTrip(src_floor, dest_floor,name);
        mtx.lock();
        cout<<schedule_msg<<" \n";
        mtx.unlock();
    }

};

class SystemAdmin{
    public:
    void initialiseElevatorSystem(vector<Elevator*> elevators, vector<Floor*> floors)
    {
        ElevatorService::getInstance(floors, elevators);
    }
    
    void removeElevator(int elevatorId){
        ElevatorService *eService = ElevatorService::findInstance();
        eService->removeAElevator(elevatorId);
    }
    void addElevator(int elevatorId){
        ElevatorService *eService = ElevatorService::findInstance();
        eService->addAElevator(elevatorId);
    }
    void disableFloor(int elevatorId, int floorId){
        ElevatorService *eService = ElevatorService::findInstance();
        eService->disableAFloor(elevatorId, floorId);
    }

    void enableFloor(int elevatorId, int floorId){
        ElevatorService *eService = ElevatorService::findInstance();
        eService->enableAFloor(elevatorId, floorId);
    }

};
void requests(Client* client){
    int k=26;
    char c='a';
    int src,dest;
    while(k--){
        // cin>>src>>dest;
        // client->requestElevator(src,dest,c);
        client->requestElevator((rand())%5,(rand())%5, c);
        c++;
        sleep(2);
    }
}

int main() {
    SystemAdmin* admin=new SystemAdmin();
    
    vector<Elevator*> elevators;
    vector<Floor*> floors;

    for(int i=1;i<3;i++){
        elevators.push_back(new Elevator(i,1));
    }
    for(int i=0;i<5;i++){
        floors.push_back(new Floor(i));
    }
    admin->initialiseElevatorSystem(elevators, floors);
    admin->addElevator(5);
    admin->disableFloor(5,2);

    Client* client=new Client();

    // Thread to queue requests
    std::thread t(ElevatorService::serviceRequests);

    // Thread to assign requests to elevators
    std::thread t1(requests, client);

    // Thread to run elevators
    std::thread t2(ElevatorService::startRunningElevators);
    
    t1.join();
    t2.join();
    t.join();

    return 0;
}