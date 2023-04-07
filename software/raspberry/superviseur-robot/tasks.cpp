/*
 * Copyright (C) 2018 dimercur
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tasks.h"
#include <stdexcept>

// Déclaration des priorités des taches
#define PRIORITY_TSERVER 30
#define PRIORITY_TOPENCOMROBOT 20
#define PRIORITY_TMOVE 20
#define PRIORITY_TSENDTOMON 22
#define PRIORITY_TRECEIVEFROMMON 25
#define PRIORITY_TSTARTROBOT 20
#define PRIORITY_TCAMERA 21

/*
 * Some remarks:
 * 1- This program is mostly a template. It shows you how to create tasks, semaphore
 *   message queues, mutex ... and how to use them
 * 
 * 2- semDumber is, as name say, useless. Its goal is only to show you how to use semaphore
 * 
 * 3- Data flow is probably not optimal
 * 
 * 4- Take into account that ComRobot::Write will block your task when serial buffer is full,
 *   time for internal buffer to flush
 * 
 * 5- Same behavior existe for ComMonitor::Write !
 * 
 * 6- When you want to write something in terminal, use cout and terminate with endl and flush
 * 
 * 7- Good luck !
 */

/**
 * @brief Initialisation des structures de l'application (tâches, mutex, 
 * semaphore, etc.)
 */
void Tasks::Init() {
    int status;
    int err;
    demandesMoniteur.isCameraOpen = false;
    demandesMoniteur.arene = false;
    demandesMoniteur.positionRobot = false;
    
    /**************************************************************************************/
    /* 	Mutex creation                                                                    */
    /**************************************************************************************/
    if (err = rt_mutex_create(&mutex_monitor, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_robot, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_robotStarted, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_move, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_camera, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_demandesMoniteur, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_getArena, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_mutex_create(&mutex_getImage, NULL)) {
        cerr << "Error mutex create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Mutexes created successfully" << endl << flush;

    /**************************************************************************************/
    /* 	Semaphors creation       							  */
    /**************************************************************************************/
    if (err = rt_sem_create(&sem_barrier, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_openComRobot, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_serverOk, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_sem_create(&sem_startRobot, NULL, 0, S_FIFO)) {
        cerr << "Error semaphore create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Semaphores created successfully" << endl << flush;

    /**************************************************************************************/
    /* Tasks creation                                                                     */
    /**************************************************************************************/
    if (err = rt_task_create(&th_server, "th_server", 0, PRIORITY_TSERVER, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_sendToMon, "th_sendToMon", 0, PRIORITY_TSENDTOMON, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_receiveFromMon, "th_receiveFromMon", 0, PRIORITY_TRECEIVEFROMMON, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_openComRobot, "th_openComRobot", 0, PRIORITY_TOPENCOMROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_startRobot, "th_startRobot", 0, PRIORITY_TSTARTROBOT, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_move, "th_move", 0, PRIORITY_TMOVE, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_create(&th_battery, "th_battery", 0, PRIORITY_TMOVE, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE); 
    }
    if (err = rt_task_create(&th_periodicCamera, "th_periodicCamera", 0, PRIORITY_TCAMERA, 0)) {
        cerr << "Error task create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE); 
    }
    cout << "Tasks created successfully" << endl << flush;
    

    /**************************************************************************************/
    /* Message queues creation                                                            */
    /**************************************************************************************/
    if ((err = rt_queue_create(&q_messageToMon, "q_messageToMon", sizeof (Message*)*50, Q_UNLIMITED, Q_FIFO)) < 0) {
        cerr << "Error msg queue create: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Queues created successfully" << endl << flush;
    
    
    rt_mutex_acquire(&mutex_getArena, TM_INFINITE);
        arena = new Arena();
    rt_mutex_release(&mutex_getArena);
}

/**
 * @brief Démarrage des tâches
 */
void Tasks::Run() {
    rt_task_set_priority(NULL, T_LOPRIO);
    int err;

    if (err = rt_task_start(&th_server, (void(*)(void*)) & Tasks::ServerTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_sendToMon, (void(*)(void*)) & Tasks::SendToMonTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_receiveFromMon, (void(*)(void*)) & Tasks::ReceiveFromMonTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_openComRobot, (void(*)(void*)) & Tasks::OpenComRobot, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_startRobot, (void(*)(void*)) & Tasks::StartRobotTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_move, (void(*)(void*)) & Tasks::MoveTask, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_battery, (void(*)(void*)) & Tasks::Battery, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    if (err = rt_task_start(&th_periodicCamera, (void(*)(void*)) & Tasks::PeriodicCamera, this)) {
        cerr << "Error task start: " << strerror(-err) << endl << flush;
        exit(EXIT_FAILURE);
    }
    cout << "Tasks launched" << endl << flush;
}

/**
 * @brief Arrêt des tâches
 */
void Tasks::Stop() {
    monitor.Close();
    robot.Close();
}

/**
 */
void Tasks::Join() {
    cout << "Tasks synchronized" << endl << flush;
    rt_sem_broadcast(&sem_barrier);
    pause();
}

/**
 * @brief Thread handling server communication with the monitor.
 */
void Tasks::ServerTask(void *arg) {
    int status;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are started)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task server starts here                                                        */
    /**************************************************************************************/
    rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
    status = monitor.Open(SERVER_PORT);
    rt_mutex_release(&mutex_monitor);

    cout << "Open server on port " << (SERVER_PORT) << " (" << status << ")" << endl;

    if (status < 0) throw std::runtime_error {
        "Unable to start server on port " + std::to_string(SERVER_PORT)
    };
    monitor.AcceptClient(); // Wait the monitor client
    cout << "Rock'n'Roll baby, client accepted!" << endl << flush;
    rt_sem_broadcast(&sem_serverOk);
}

/**
 * @brief Thread sending data to monitor.
 */
void Tasks::SendToMonTask(void* arg) {
    Message *msg;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);

    /**************************************************************************************/
    /* The task sendToMon starts here                                                     */
    /**************************************************************************************/
    rt_sem_p(&sem_serverOk, TM_INFINITE);

    while (1) {
        cout << "wait msg to send" << endl << flush;
        msg = ReadInQueue(&q_messageToMon);
        cout << "Send msg to mon: " << msg->ToString() << endl << flush;
        rt_mutex_acquire(&mutex_monitor, TM_INFINITE);
        monitor.Write(msg); // The message is deleted with the Write
        rt_mutex_release(&mutex_monitor);
    }
}

/**
 * @brief Thread receiving data from monitor.
 */
void Tasks::ReceiveFromMonTask(void *arg) {
    Message *msgRcv;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task receiveFromMon starts here                                                */
    /**************************************************************************************/
    rt_sem_p(&sem_serverOk, TM_INFINITE);
    cout << "Received message from monitor activated" << endl << flush;

    while (1) {
        msgRcv = monitor.Read();
        cout << "Rcv <= " << msgRcv->ToString() << endl << flush;

        if (msgRcv->CompareID(MESSAGE_MONITOR_LOST)) {
            delete(msgRcv);
            exit(-1);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_COM_OPEN)) {
            rt_sem_v(&sem_openComRobot);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_START_WITHOUT_WD)) {
            rt_sem_v(&sem_startRobot);
        } else if (msgRcv->CompareID(MESSAGE_ROBOT_GO_FORWARD) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_BACKWARD) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_LEFT) ||
                msgRcv->CompareID(MESSAGE_ROBOT_GO_RIGHT) ||
                msgRcv->CompareID(MESSAGE_ROBOT_STOP)) {

            rt_mutex_acquire(&mutex_move, TM_INFINITE);
            move = msgRcv->GetID();
            rt_mutex_release(&mutex_move);
        } 
        else if (msgRcv->CompareID(MESSAGE_CAM_OPEN)) {
            rt_mutex_acquire(&mutex_camera, TM_INFINITE);
            rt_mutex_acquire(&mutex_demandesMoniteur, TM_INFINITE);
            if (camera == NULL) {
                camera = new Camera();
            }
            if (!demandesMoniteur.isCameraOpen) {
                demandesMoniteur.isCameraOpen = camera->Open();
            } else {

            }
            rt_mutex_release(&mutex_demandesMoniteur);
            rt_mutex_release(&mutex_camera);
        } else if (msgRcv->CompareID(MESSAGE_CAM_CLOSE)) {
            rt_mutex_acquire(&mutex_camera, TM_INFINITE);
            rt_mutex_acquire(&mutex_demandesMoniteur, TM_INFINITE);
            if (camera != NULL && demandesMoniteur.isCameraOpen) {
                camera->Close();
                demandesMoniteur.isCameraOpen = false;
            }
            rt_mutex_release(&mutex_demandesMoniteur);
            rt_mutex_release(&mutex_camera);
        } else if (msgRcv->CompareID(MESSAGE_CAM_ASK_ARENA)) {
            rt_mutex_acquire(&mutex_demandesMoniteur, TM_INFINITE);      
            demandesMoniteur.arene = true;
            rt_mutex_release(&mutex_demandesMoniteur);
        } else if (msgRcv->CompareID(MESSAGE_CAM_ARENA_CONFIRM)) {
            rt_mutex_acquire(&mutex_demandesMoniteur, TM_INFINITE);
            demandesMoniteur.arene = false;
            rt_mutex_release(&mutex_demandesMoniteur);
        } else if (msgRcv->CompareID(MESSAGE_CAM_ARENA_INFIRM)) {
            rt_mutex_acquire(&mutex_demandesMoniteur, TM_INFINITE);
            demandesMoniteur.arene = true;
            rt_mutex_release(&mutex_demandesMoniteur);

            rt_mutex_acquire(&mutex_getArena, TM_INFINITE);
            arena = new Arena();
            rt_mutex_release(&mutex_getArena);
        } else if (msgRcv->CompareID(MESSAGE_CAM_POSITION_COMPUTE_START)) {
            rt_mutex_acquire(&mutex_demandesMoniteur, TM_INFINITE);
            demandesMoniteur.positionRobot = true;
            rt_mutex_release(&mutex_demandesMoniteur);
        } else if (msgRcv->CompareID(MESSAGE_CAM_POSITION_COMPUTE_STOP)) {
            rt_mutex_acquire(&mutex_demandesMoniteur, TM_INFINITE);
            demandesMoniteur.positionRobot = false;
            rt_mutex_release(&mutex_demandesMoniteur);
        }
    }
        delete(msgRcv); // mus be deleted manually, no consumer
}

/**
 * @brief Thread opening communication with the robot.
 */
void Tasks::OpenComRobot(void *arg) {
    int status;
    int err;

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task openComRobot starts here                                                  */
    /**************************************************************************************/
    while (1) {
        rt_sem_p(&sem_openComRobot, TM_INFINITE);
        cout << "Open serial com (";
        rt_mutex_acquire(&mutex_robot, TM_INFINITE);
        status = robot.Open();
        rt_mutex_release(&mutex_robot);
        cout << status;
        cout << ")" << endl << flush;

        Message * msgSend;
        if (status < 0) {
            msgSend = new Message(MESSAGE_ANSWER_NACK);
        } else {
            msgSend = new Message(MESSAGE_ANSWER_ACK);
        }
        WriteInQueue(&q_messageToMon, msgSend); // msgSend will be deleted by sendToMon
    }
}

/**
 * @brief Thread starting the communication with the robot.
 */
void Tasks::StartRobotTask(void *arg) {
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task startRobot starts here                                                    */
    /**************************************************************************************/
    while (1) {
        Message * msgSend;
        rt_sem_p(&sem_startRobot, TM_INFINITE);
        cout << "Start robot without watchdog (";
        rt_mutex_acquire(&mutex_robot, TM_INFINITE);
        msgSend = write2(robot.StartWithoutWD(), &robot);
        rt_mutex_release(&mutex_robot);
        cout << msgSend->GetID();
        cout << ")" << endl;

        cout << "Movement answer: " << msgSend->ToString() << endl << flush;
        WriteInQueue(&q_messageToMon, msgSend);  // msgSend will be deleted by sendToMon

        if (msgSend->GetID() == MESSAGE_ANSWER_ACK) {
            rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
            robotStarted = 1;
            rt_mutex_release(&mutex_robotStarted);
        }
    }
}

/**
 * @brief Thread handling control of the robot.
 */
void Tasks::MoveTask(void *arg) {
    int rs;
    int cpMove;
    
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/
    rt_task_set_periodic(NULL, TM_NOW, 100000000);

    while (1) {
        rt_task_wait_period(NULL);
        cout << "Periodic movement update" << endl << flush;;
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        rs = robotStarted;
        rt_mutex_release(&mutex_robotStarted);
        if (rs == 1) {
            rt_mutex_acquire(&mutex_move, TM_INFINITE);
            cpMove = move;
            rt_mutex_release(&mutex_move);
            
            cout << " move: " << cpMove << endl << flush;;
            
            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            write2(new Message((MessageID)cpMove), &robot);
            rt_mutex_release(&mutex_robot);
        }
        cout << endl << flush;
    }
}

/**
 * Write a message in a given queue
 * @param queue Queue identifier
 * @param msg Message to be stored
 */
void Tasks::WriteInQueue(RT_QUEUE *queue, Message *msg) {
    int err;
    if ((err = rt_queue_write(queue, (const void *) &msg, sizeof ((const void *) &msg), Q_NORMAL)) < 0) {
        cerr << "Write in queue failed: " << strerror(-err) << endl << flush;
        //throw std::runtime_error{"Error in write in queue"};
    }
}

/**
 * Read a message from a given queue, block if empty
 * @param queue Queue identifier
 * @return Message read
 */
Message *Tasks::ReadInQueue(RT_QUEUE *queue) {
    int err;
    Message *msg;

    if ((err = rt_queue_read(queue, &msg, sizeof ((void*) &msg), TM_INFINITE)) < 0) {
        cout << "Read in queue failed: " << strerror(-err) << endl << flush;
        throw std::runtime_error{"Error in read in queue"};
    }/** else {
        cout << "@msg :" << msg << endl << flush;
    } /**/

    return msg;
}



void Tasks::Battery(void *arg) {

    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/
    rt_task_set_periodic(NULL, TM_NOW, 500000000); // 0.5s

    while (1) {
        rt_task_wait_period(NULL);
        
        rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
        if (robotStarted == 1) {
            cout << "Periodic battery update" << endl << flush;
            rt_mutex_acquire(&mutex_robot, TM_INFINITE);
            WriteInQueue(&q_messageToMon, 
                    write2(new Message(MESSAGE_ROBOT_BATTERY_GET), &robot)
                    );

            //monitor.Write(new Message(MESSAGE_ROBOT_BATTERY_LEVEL));
            rt_mutex_release(&mutex_robot);
        }
        rt_mutex_release(&mutex_robotStarted);
    }
}

Message* Tasks::watchdogLimit(ComRobot* r, int* c) {
    cout << "ERROR COM" << endl << flush;
    rt_mutex_acquire(&mutex_robotStarted, TM_INFINITE);
    robotStarted = 0;
    rt_mutex_release(&mutex_robotStarted);
    r->Close();
    *c=0;
    return new Message(MESSAGE_ANSWER_COM_ERROR);
}

Message* Tasks::write2(Message* m, ComRobot* r) {
    static int c = 0;
    cout << "Write2" << "[" << c << "]: " << m->ToString() << endl << flush;
    try {
        Message *mess = robot.Write(m);
        if (mess->CompareID(MESSAGE_ANSWER_ROBOT_TIMEOUT) || 
            mess->CompareID(MESSAGE_ANSWER_COM_ERROR) || 
            mess->CompareID(MESSAGE_ANSWER_ROBOT_UNKNOWN_COMMAND)) {
            c++;
            if(c>=3) {
                return watchdogLimit(r,&c);
            }
        }
        return mess;
    } catch (std::runtime_error &e) {
        c++;
        if (c>=3) {
            return watchdogLimit(r,&c);
        } 
        return new Message(MESSAGE_ANSWER_COM_ERROR);    
    }
    return new Message(MESSAGE_ANSWER_COM_ERROR);    
}

void Tasks::PeriodicCamera(void *arg) {
    cout << "Start " << __PRETTY_FUNCTION__ << endl << flush;
    // Synchronization barrier (waiting that all tasks are starting)
    rt_sem_p(&sem_barrier, TM_INFINITE);
    
    /**************************************************************************************/
    /* The task starts here                                                               */
    /**************************************************************************************/
    rt_task_set_periodic(NULL, TM_NOW, 100000000); // 0.1s

    while (1) {
        rt_task_wait_period(NULL);
        cout << "Periodic battery update" << endl << flush;
                rt_mutex_acquire(&mutex_demandesMoniteur, TM_INFINITE);
        if (demandesMoniteur.isCameraOpen) {
            rt_mutex_acquire(&mutex_camera, TM_INFINITE);
            img = new Img(camera->Grab()); // TODO creer destructeur Img
            rt_mutex_release(&mutex_camera);

            cout << "arena" << endl << flush;

            rt_mutex_acquire(&mutex_getArena, TM_INFINITE);
            if (demandesMoniteur.arene && arena->IsEmpty()) {
                arena = new Arena(img->SearchArena());
            }
            if (!arena->IsEmpty()) {
                img->DrawArena(*arena);
            }
            if (demandesMoniteur.positionRobot) {
                img->DrawAllRobots(img->SearchRobot(*arena));
            }

            rt_mutex_release(&mutex_getArena);

            cout << "endif arena" << endl << flush;

            WriteInQueue(&q_messageToMon,                 //if (demandesMoniteur.arene) {
                    new MessageImg(MESSAGE_CAM_IMAGE, img)
                );
        }
        rt_mutex_release(&mutex_demandesMoniteur);
    }
}