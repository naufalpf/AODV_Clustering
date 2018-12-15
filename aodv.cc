#include <aodv/aodv.h>
#include <aodv/aodv_packet.h>
#include <random.h>
#include <cmu-trace.h>
#include <iostream>
//#include <energy-model.h>

#define max(a, b) ((a) > (b) ? (a) : (b))
#define CURRENT_TIME Scheduler::instance().clock()

//#define DEBUG
//#define ERROR

int count_neighbour[1000];    // menghitung jumlah tetangga tiap node
int count_mode[1000];        // menghitung modus
int masuk[1000];
//count_neighbour = new int[0];
int nodes_count=0;            // modifikasi jumlah node terhitung
int th = 0;                 // menghitung threshold
int old_th;

#ifdef DEBUG
static int route_request = 0;
#endif

/*
  TCL Hooks
*/

int hdr_aodv::offset_;
static class AODVHeaderClass : public PacketHeaderClass
{
public:
  AODVHeaderClass() : PacketHeaderClass("PacketHeader/AODV",
                                        sizeof(hdr_all_aodv))
  {
    bind_offset(&hdr_aodv::offset_);
  }
} class_rtProtoAODV_hdr;

static class AODVclass : public TclClass
{
public:
  AODVclass() : TclClass("Agent/AODV") {}
  TclObject *create(int argc, const char *const *argv)
  {
    assert(argc == 5);
    //return (new AODV((nsaddr_t) atoi(argv[4])));
    return (new AODV((nsaddr_t)Address::instance().str2addr(argv[4])));
  }
} class_rtProtoAODV;

int AODV::command(int argc, const char *const *argv)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // menampilkan waktu
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::command", now);
  fclose(fp);

  if (argc == 2)
  {
    Tcl &tcl = Tcl::instance();

    if (strncasecmp(argv[1], "id", 2) == 0)
    {
      tcl.resultf("%d", index);
      return TCL_OK;
    }

    if (strncasecmp(argv[1], "start", 2) == 0)
    {
      btimer.handle((Event *)0);

//#ifndef AODV_LINK_LAYER_DETECTION
      htimer.handle((Event *)0);
      mtimer.handle((Event *)0);
      ntimer.handle((Event *)0);
//#endif // LINK LAYER DETECTION

      rtimer.handle((Event *)0);
      return TCL_OK;
    }
  }
  else if (argc == 3)
  {
    if (strcmp(argv[1], "index") == 0)
    {
      index = atoi(argv[2]);
      return TCL_OK;
    }

    else if (strcmp(argv[1], "log-target") == 0 || strcmp(argv[1], "tracetarget") == 0)
    {
      logtarget = (Trace *)TclObject::lookup(argv[2]);
      if (logtarget == 0)
        return TCL_ERROR;
      return TCL_OK;
    }
    else if (strcmp(argv[1], "drop-target") == 0)
    {
      int stat = rqueue.command(argc, argv);
      if (stat != TCL_OK)
        return stat;
      return Agent::command(argc, argv);
    }
    else if (strcmp(argv[1], "if-queue") == 0)
    {
      ifqueue = (PriQueue *)TclObject::lookup(argv[2]);

      if (ifqueue == 0)
        return TCL_ERROR;
      return TCL_OK;
    }
    else if (strcmp(argv[1], "port-dmux") == 0)
    {
      dmux_ = (PortClassifier *)TclObject::lookup(argv[2]);
      if (dmux_ == 0)
      {
        fprintf(stderr, "%s: %s lookup of %s failed\n", __FILE__,
                argv[1], argv[2]);
        return TCL_ERROR;
      }
      return TCL_OK;
    }
  }
  return Agent::command(argc, argv);
}

/* 
   Constructor
*/

AODV::AODV(nsaddr_t id) : Agent(PT_AODV),
                          btimer(this), htimer(this), mtimer(this), ntimer(this),
                          rtimer(this), lrtimer(this), rqueue()
{

  index = id;
  seqno = 2;
  bid = 1;

                         // inisiasi manet dan energi
  xpos = 0.0; 
  ypos = 0.0; 
  zpos = 0.0; 
  MobileNode *iNode; 
  iEnergy = 1.0; 

  LIST_INIT(&nbhead);
  LIST_INIT(&bihead);

  logtarget = 0;
  ifqueue = 0;
  CH_ID = -1;          // modifikasi inisiasi penyimpanan cluster head
}

/*
  Timers
*/

void BroadcastTimer::handle(Event *)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi BroadcastTimer::handle", now);
  fclose(fp);

  agent->id_purge();
  Scheduler::instance().schedule(this, &intr, BCAST_ID_SAVE);
}

void HelloTimer::handle(Event *)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi HelloTimer::handle", now);
  fclose(fp);

  agent->sendHello();
  double interval = MinHelloInterval +
                    ((MaxHelloInterval - MinHelloInterval) * Random::uniform());

  assert(interval >= 0);

 Scheduler::instance().schedule(this, &intr, interval);
}

void Modif::handle(Event *)                   // modifikasi perhitungan jumlah node tetangga
{
  double now = Scheduler::instance().clock(); 
  FILE *fp;
  double interval = 5.0;
  
  int run = now / interval;
  if (masuk[run]==0) masuk[run]=0;

  if(now > 0.000000 && masuk[run]==0)
  {
    masuk[run]=100;                               // melakukan pengurutan tetangga terbanyak
    
    for(int i=0;i<nodes_count;i++)
    {
     for(int j=(i+1);j<nodes_count;j++)
      {
       if(count_neighbour[i]>count_neighbour[j])
        {
         int tmp;
         tmp=count_neighbour[i];
         count_neighbour[i]=count_neighbour[j];     // melakukan pengurutan node dengan tetangga terbanyak
         count_neighbour[j]=tmp;
        }
      }
    }

                                                    // menghitung berapa kali muncul tiap angka
    for(int i=0;i<nodes_count;i++)
    {
     count_mode[i]=0;
      for(int j=0;j<nodes_count;j++)
      {
       if(count_neighbour[i]==count_neighbour[j])
         {
          count_mode[i]++;
         }
      }
    }

    
    for(int i=0;i<nodes_count;i++)                      // menentukan nilai yang paling sering muncul
    {
      if(count_mode[i]>th)
      {
        th=count_mode[i];
      }
    }

    
    old_th = th;                        // threshold digunakan sebagai acuan pencarian cluster head
    // if(old_th != th)
    // {
      fp = fopen("debug.txt", "a");
      fprintf(fp, "\n %f fungsi Modif, th: %d", now, th);
      fclose(fp);  
    
      fp = fopen("threshold.txt", "a");
      fprintf(fp, "\n %f th : %d, run %d", now, th, run);
      fclose(fp);
    // }
  }

  
  Scheduler::instance().schedule(this, &intr, interval);
}

void NeighborTimer::handle(Event *)
{

  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi NeighborTimer::handle", now);
  fclose(fp);

  agent->nb_purge();
  Scheduler::instance().schedule(this, &intr, HELLO_INTERVAL);
}

void RouteCacheTimer::handle(Event *)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi RouteCacheTimer::handle", now);
  fclose(fp);

  agent->rt_purge();
#define FREQUENCY 0.5 // sec
  Scheduler::instance().schedule(this, &intr, FREQUENCY);
}

void LocalRepairTimer::handle(Event *p)
{ // SRD: 5/4/99
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi LocalRepairTimer::handle", now);
  fclose(fp);

  aodv_rt_entry *rt;
  struct hdr_ip *ih = HDR_IP((Packet *)p);

  /* you get here after the timeout in a local repair attempt */
  /*  fprintf(stderr, "%s\n", __FUNCTION__); */

  rt = agent->rtable.rt_lookup(ih->daddr());

  if (rt && rt->rt_flags != RTF_UP)
  {
    // route is yet to be repaired
    // I will be conservative and bring down the route
    // and send route errors upstream.
    /* The following assert fails, not sure why */
    /* assert (rt->rt_flags == RTF_IN_REPAIR); */

    //rt->rt_seqno++;
    agent->rt_down(rt);
    // send RERR
#ifdef DEBUG
    fprintf(stderr, "Dst - %d, failed local repair\n", rt->rt_dst);
#endif
  }
  Packet::free((Packet *)p);
}

/*
   Broadcast ID Management  Functions
*/

void AODV::id_insert(nsaddr_t id, u_int32_t bid)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::id_insert", now);
  fclose(fp);

  BroadcastID *b = new BroadcastID(id, bid);

  assert(b);
  b->expire = CURRENT_TIME + BCAST_ID_SAVE;
  LIST_INSERT_HEAD(&bihead, b, link);
}

/* SRD */
bool AODV::id_lookup(nsaddr_t id, u_int32_t bid)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::id_lookup", now);
  fclose(fp);

  BroadcastID *b = bihead.lh_first;

  // Search the list for a match of source and bid
  for (; b; b = b->link.le_next)
  {
    if ((b->src == id) && (b->id == bid))
      return true;
  }
  return false;
}

void AODV::id_purge()
{
  //print fungsi
  double nows = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::id_purge", nows);
  fclose(fp);

  BroadcastID *b = bihead.lh_first;
  BroadcastID *bn;
  double now = CURRENT_TIME;

  for (; b; b = bn)
  {
    bn = b->link.le_next;
    if (b->expire <= now)
    {
      LIST_REMOVE(b, link);
      delete b;
    }
  }
}

/*
  Helper Functions
*/

double
AODV::PerHopTime(aodv_rt_entry *rt)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::PerHopTime", now);
  fclose(fp);

  int num_non_zero = 0, i;
  double total_latency = 0.0;

  if (!rt)
    return ((double)NODE_TRAVERSAL_TIME);

  for (i = 0; i < MAX_HISTORY; i++)
  {
    if (rt->rt_disc_latency[i] > 0.0)
    {
      num_non_zero++;
      total_latency += rt->rt_disc_latency[i];
    }
  }
  if (num_non_zero > 0)
    return (total_latency / (double)num_non_zero);
  else
    return ((double)NODE_TRAVERSAL_TIME);
}

/*
  Link Failure Management Functions
*/

static void
aodv_rt_failed_callback(Packet *p, void *arg)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi aodv_rt_failed_callback", now);
  fclose(fp);

  ((AODV *)arg)->rt_ll_failed(p);
}

/*
 * This routine is invoked when the link-layer reports a route failed.
 */
void AODV::rt_ll_failed(Packet *p)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::rt_ll_failed", now);
  fclose(fp);

  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  aodv_rt_entry *rt;
  nsaddr_t broken_nbr = ch->next_hop_;

#ifndef AODV_LINK_LAYER_DETECTION
  drop(p, DROP_RTR_MAC_CALLBACK);
#else

  /*
  * Non-data packets and Broadcast Packets can be dropped.
  */
  if (!DATA_PACKET(ch->ptype()) ||
      (u_int32_t)ih->daddr() == IP_BROADCAST)
  {
    drop(p, DROP_RTR_MAC_CALLBACK);
    return;
  }
  log_link_broke(p);
  if ((rt = rtable.rt_lookup(ih->daddr())) == 0)
  {
    drop(p, DROP_RTR_MAC_CALLBACK);
    return;
  }
  log_link_del(ch->next_hop_);

#ifdef AODV_LOCAL_REPAIR
  /* if the broken link is closer to the dest than source, 
     attempt a local repair. Otherwise, bring down the route. */

  if (ch->num_forwards() > rt->rt_hops)
  {
    local_rt_repair(rt, p); // local repair
    // retrieve all the packets in the ifq using this link,
    // queue the packets for which local repair is done,
    return;
  }
  else
#endif // LOCAL REPAIR

  {
    drop(p, DROP_RTR_MAC_CALLBACK);
    // Do the same thing for other packets in the interface queue using the
    // broken link -Mahesh
    while ((p = ifqueue->filter(broken_nbr)))
    {
      drop(p, DROP_RTR_MAC_CALLBACK);
    }
    nb_delete(broken_nbr);
  }

#endif // LINK LAYER DETECTION
}

void AODV::handle_link_failure(nsaddr_t id)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::handle_link_failure", now);
  fclose(fp);

  aodv_rt_entry *rt, *rtn;
  Packet *rerr = Packet::alloc();
  struct hdr_aodv_error *re = HDR_AODV_ERROR(rerr);

  re->DestCount = 0;
  for (rt = rtable.head(); rt; rt = rtn)
  { // for each rt entry
    rtn = rt->rt_link.le_next;
    if ((rt->rt_hops != INFINITY2) && (rt->rt_nexthop == id))
    {
      assert(rt->rt_flags == RTF_UP);
      assert((rt->rt_seqno % 2) == 0);
      rt->rt_seqno++;
      re->unreachable_dst[re->DestCount] = rt->rt_dst;
      re->unreachable_dst_seqno[re->DestCount] = rt->rt_seqno;
#ifdef DEBUG
      fprintf(stderr, "%s(%f): %d\t(%d\t%u\t%d)\n", __FUNCTION__, CURRENT_TIME,
              index, re->unreachable_dst[re->DestCount],
              re->unreachable_dst_seqno[re->DestCount], rt->rt_nexthop);
#endif // DEBUG
      re->DestCount += 1;
      rt_down(rt);
    }
    // remove the lost neighbor from all the precursor lists
    rt->pc_delete(id);
  }

  if (re->DestCount > 0)
  {
#ifdef DEBUG
    fprintf(stderr, "%s(%f): %d\tsending RERR...\n", __FUNCTION__, CURRENT_TIME, index);
#endif // DEBUG
    sendError(rerr, false);
  }
  else
  {
    Packet::free(rerr);
  }
}

void AODV::local_rt_repair(aodv_rt_entry *rt, Packet *p)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::local_rt_repair", now);
  fclose(fp);

#ifdef DEBUG
  fprintf(stderr, "%s: Dst - %d\n", __FUNCTION__, rt->rt_dst);
#endif
  // Buffer the packet
  rqueue.enque(p);

  // mark the route as under repair
  rt->rt_flags = RTF_IN_REPAIR;

  sendRequest(rt->rt_dst);

  // set up a timer interrupt
  Scheduler::instance().schedule(&lrtimer, p->copy(), rt->rt_req_timeout);
}

void AODV::rt_update(aodv_rt_entry *rt, u_int32_t seqnum, u_int16_t metric,
                     nsaddr_t nexthop, double expire_time)
{

  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::rt_update", now);
  fclose(fp);

  rt->rt_seqno = seqnum;
  rt->rt_hops = metric;
  rt->rt_flags = RTF_UP;
  rt->rt_nexthop = nexthop;
  rt->rt_expire = expire_time;
}

void AODV::rt_down(aodv_rt_entry *rt)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::rt_down", now);
  fclose(fp);

  /*
   *  Make sure that you don't "down" a route more than once.
   */

  if (rt->rt_flags == RTF_DOWN)
  {
    return;
  }

  // assert (rt->rt_seqno%2); // is the seqno odd?
  rt->rt_last_hop_count = rt->rt_hops;
  rt->rt_hops = INFINITY2;
  rt->rt_flags = RTF_DOWN;
  rt->rt_nexthop = 0;
  rt->rt_expire = 0;

} /* rt_down function */

/*
  Route Handling Functions
*/

void AODV::rt_resolve(Packet *p)
{
  //print fungsi
  double nows = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::rt_resolve", nows);
  fclose(fp);

  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  aodv_rt_entry *rt;

  /*
  *  Set the transmit failure callback.  That
  *  won't change.
  */
  ch->xmit_failure_ = aodv_rt_failed_callback;
  ch->xmit_failure_data_ = (void *)this;
  rt = rtable.rt_lookup(ih->daddr());
  if (rt == 0)
  {
    rt = rtable.rt_add(ih->daddr());
  }

  /*
  * If the route is up, forward the packet 
  */

  if (rt->rt_flags == RTF_UP)
  {
    assert(rt->rt_hops != INFINITY2);
    forward(rt, p, NO_DELAY);
  }
  /*
  *  if I am the source of the packet, then do a Route Request.
  */
  else if (ih->saddr() == index)
  {
    rqueue.enque(p);
    sendRequest(rt->rt_dst);
  }
  /*
  * A local repair is in progress. Buffer the packet. 
  */
  else if (rt->rt_flags == RTF_IN_REPAIR)
  {
    rqueue.enque(p);
  }

  /*
  * I am trying to forward a packet for someone else to which
  * I don't have a route.
  */
  else
  {
    Packet *rerr = Packet::alloc();
    struct hdr_aodv_error *re = HDR_AODV_ERROR(rerr);
    /* 
  * For now, drop the packet and send error upstream.
  * Now the route errors are broadcast to upstream
  * neighbors - Mahesh 09/11/99
  */

    assert(rt->rt_flags == RTF_DOWN);
    re->DestCount = 0;
    re->unreachable_dst[re->DestCount] = rt->rt_dst;
    re->unreachable_dst_seqno[re->DestCount] = rt->rt_seqno;
    re->DestCount += 1;
#ifdef DEBUG
    fprintf(stderr, "%s: sending RERR...\n", __FUNCTION__);
#endif
    sendError(rerr, false);

    drop(p, DROP_RTR_NO_ROUTE);
  }
}

void AODV::rt_purge()
{
  //print fungsi
  double nows = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::rt_purge", nows);
  fclose(fp);

  aodv_rt_entry *rt, *rtn;
  double now = CURRENT_TIME;
  double delay = 0.0;
  Packet *p;

  for (rt = rtable.head(); rt; rt = rtn)
  { // for each rt entry
    rtn = rt->rt_link.le_next;
    if ((rt->rt_flags == RTF_UP) && (rt->rt_expire < now))
    {
      // if a valid route has expired, purge all packets from
      // send buffer and invalidate the route.
      assert(rt->rt_hops != INFINITY2);
      while ((p = rqueue.deque(rt->rt_dst)))
      {
#ifdef DEBUG
        fprintf(stderr, "%s: calling drop()\n",
                __FUNCTION__);
#endif // DEBUG
        drop(p, DROP_RTR_NO_ROUTE);
      }
      rt->rt_seqno++;
      assert(rt->rt_seqno % 2);
      rt_down(rt);
    }
    else if (rt->rt_flags == RTF_UP)
    {
      // If the route is not expired,
      // and there are packets in the sendbuffer waiting,
      // forward them. This should not be needed, but this extra
      // check does no harm.
      assert(rt->rt_hops != INFINITY2);
      while ((p = rqueue.deque(rt->rt_dst)))
      {
        forward(rt, p, delay);
        delay += ARP_DELAY;
      }
    }
    else if (rqueue.find(rt->rt_dst))
      // If the route is down and
      // if there is a packet for this destination waiting in
      // the sendbuffer, then send out route request. sendRequest
      // will check whether it is time to really send out request
      // or not.
      // This may not be crucial to do it here, as each generated
      // packet will do a sendRequest anyway.

      sendRequest(rt->rt_dst);
  }
}

/*
  Packet Reception Routines
*/

void AODV::recv(Packet *p, Handler *)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::recv ", now);
  fclose(fp);

  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);

  assert(initialized());
  //assert(p->incoming == 0);
  // XXXXX NOTE: use of incoming flag has been depracated; In order to track direction of pkt flow, direction_ in hdr_cmn is used instead. see packet.h for details.

  if (ch->ptype() == PT_AODV)
  {
    ih->ttl_ -= 1;
    recvAODV(p);
    return;
  }

  /*
  *  Must be a packet I'm originating...
  */
  if ((ih->saddr() == index) && (ch->num_forwards() == 0))
  {

    if (ch->ptype() != PT_TCP && ch->ptype() != PT_ACK)
    {
      ch->size() += IP_HDR_LEN;
    }
    // handle broadcast
    if ((u_int32_t)ih->daddr() != IP_BROADCAST)
    {
      ih->ttl_ = NETWORK_DIAMETER;
    }
  }
  /*
  *  Modifikasi Mendapatkan paket yang telah dikirimkan, routing loop
  */
  else if (ih->saddr() == index)
  {
    drop(p, DROP_RTR_ROUTE_LOOP);
    return;
  }
  /*
  *  
  */
  else
  {
    /*
  *  Check the TTL.  If it is zero, then discard.
  */
    if (--ih->ttl_ == 0)
    {
      drop(p, DROP_RTR_TTL);
      return;
    }
  }
  // Added by Parag Dadhania && John Novatnack to handle broadcasting
  if ((u_int32_t)ih->daddr() != IP_BROADCAST)
    rt_resolve(p);
  else
    forward((aodv_rt_entry *)0, p, NO_DELAY);
}

void AODV::recvAODV(Packet *p)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::recvAODV", now);
  fclose(fp);

  struct hdr_aodv *ah = HDR_AODV(p);

  assert(HDR_IP(p)->sport() == RT_PORT);
  assert(HDR_IP(p)->dport() == RT_PORT);

  /*
  * Incoming Packets.
  */
  switch (ah->ah_type)
  {

  case AODVTYPE_RREQ:
    recvRequest(p);
    break;

  case AODVTYPE_RREP:
    recvReply(p);
    break;

  case AODVTYPE_RERR:
    recvError(p);
    break;

  case AODVTYPE_HELLO:
    recvHello(p);
    break;

  default:
    fprintf(stderr, "Invalid AODV type (%x)\n", ah->ah_type);
    exit(1);
  }
}

void AODV::calculateCHID()   // modifikasi perhitungan threshold pencarian kandidat cluster head
{
  int max_degree = -1;                    // modifikasi untuk menentukan berapa minimal tetangga agar suatu node dikatakan cluster head

  for (int i = 0; i < sizeof(count_neighbour) / sizeof(*count_neighbour); i++) {
    if (count_neighbour[i] > max_degree)
      max_degree = count_neighbour[i];
  }

  if (count_neighbour[index] == max_degree) {
    CH_ID = index;
  }
  else {
    CH_ID = -1;
  }

  FILE *fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::calculateCHID di node %d degreenya: %d, max_degree: %d, CH_ID: %d", CURRENT_TIME, index, count_neighbour[index], max_degree, CH_ID);
  fclose(fp);
}

void AODV::recvRequest(Packet *p)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::recvRequest sedang berada di node: %d jumlah tetangga %d ", now, index,count_neighbour[index]);
  fclose(fp);

  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_aodv_request *rq = HDR_AODV_REQUEST(p);
  aodv_rt_entry *rt;

   // modifikasi energi
  iNode=    (MobileNode *) (Node::get_node_by_address (index) ); 
  xpos=     iNode->X(); 
  ypos=     iNode->Y(); 
  iEnergy=  iNode->energy_model()->energy();


  #ifdef DEBUG
    fp = fopen("debug.txt", "a");
    fprintf(fp, "\n%.6f  fungsi Energi : saat ini di node %d dengan energy: %.4f, paket dari node %d ke node tujuan: %d dengan lifetime %.4f",  CURRENT_TIME, index, iEnergy, rq->rq_src, rq->rq_dst, rq->rq_min_life);
    fclose(fp);   
   #endif // DEBUG


  if (rq->rq_min_life > iEnergy && iEnergy > 0){
    #ifdef DEBUG
     
      fp = fopen("debug.txt", "a");
      fprintf(fp, "\n%.6f  lifetime node %d dari %.4f menjadi energi baru sebesar %.4f", CURRENT_TIME, index, rq->rq_min_life, iEnergy);
      fclose(fp);   
     #endif // DEBUG
    rq->rq_min_life=  iEnergy;
  }

  fp = fopen("debug.txt", "a");

  calculateCHID();  // modifikasi menjalankan fungsi jika node lain bukan cluster head, maka menjadi gateway 

  if (CH_ID != -1) {
    // inisiasi cluster head
    fprintf(fp, "\n %f fungsi AODV::recvRequest sedang berada di node: %d, node %d adalah node cluster head", now, index, index);
  }
  else if (CH_ID == -1) {
    if (rq->rq_cluster_head_index != -1) {
      // modifikasi gateway cooperative
      CH_ID = rq->rq_cluster_head_index;
      fprintf(fp, "\n %f fungsi AODV::recvRequest sedang berada di node: %d, node %d adalah node gateway cooperative", now, index, index);
    }
    else {
      // modifikasi gatewat alternatif
      fprintf(fp, "\n %f fungsi AODV::recvRequest sedang berada di node: %d, node %d adalah node gateway alternative", now, index, index);
    }
  }

  fclose(fp);

  // modifikasi menampilkan list tetangga tiap node
  Node* m_node = Node::get_node_by_address(this->addr());
  neighbor_list_node* my_mobile_neighbor_list;
  my_mobile_neighbor_list = m_node->neighbor_list_;
  int count = 0;

  //cout<<"Node id : " <<index<<endl;
  while(my_mobile_neighbor_list)
  {
      //cout<<"Neighbor ID:"<<my_mobile_neighbor_list->nodeid<<endl;
      count++;
      if(my_mobile_neighbor_list->next){
          my_mobile_neighbor_list=my_mobile_neighbor_list->next;
      }
      else{
          //cout<<"## Jumlah: "<<count<<endl;
          //cout<<"## Jumlah: "<<nb_node()<<endl;
          break;
      }
  }
  //cout<<"Jumlah: "<<count<<endl;
  //cout<<"**Jumlah: "<<nb_node()<<endl;

   /*
   * Drop if:
   *      - I'm the source
   *      - I recently heard this request.
   */

  if (rq->rq_src == index)
  {
#ifdef DEBUG
    fprintf(stderr, "%s: got my own REQUEST\n", __FUNCTION__);
#endif // DEBUG
    Packet::free(p);
    return;
  }

  if (id_lookup(rq->rq_src, rq->rq_bcast_id))
  {

#ifdef DEBUG
    fprintf(stderr, "%s: discarding request\n", __FUNCTION__);
#endif // DEBUG

    Packet::free(p);
    return;
  }

   //if (false){
   if (count_neighbour[index] < th && rq->rq_dst != index){  //
     
    fp = fopen("debug.txt", "a");
    fprintf(fp, "\n %f th yang dipakai : %d ", now, th);
    fclose(fp);

    fp = fopen("threshold.txt", "a");
    fprintf(fp, "\n %f th digunakan : %d", now, th);
    fclose(fp);

     Packet::free(p);
     return;
     //cout<<"Drop paket"<<endl;
   }
  
  // else {
    /*
    * Cache the broadcast ID
    */
    id_insert(rq->rq_src, rq->rq_bcast_id);
    
    /* 
    * We are either going to forward the REQUEST or generate a
    * REPLY. Before we do anything, we make sure that the REVERSE
    * route is in the route table.
    */
    aodv_rt_entry *rt0; // rt0 is the reverse route
  
    rt0 = rtable.rt_lookup(rq->rq_src);
    if (rt0 == 0)
    { /* if not in the route table */
      // create an entry for the reverse route.
      rt0 = rtable.rt_add(rq->rq_src);
    }
  
    rt0->rt_expire = max(rt0->rt_expire, (CURRENT_TIME + REV_ROUTE_LIFE));
  
    if ((rq->rq_src_seqno > rt0->rt_seqno) ||
        ((rq->rq_src_seqno == rt0->rt_seqno) &&
         (rq->rq_hop_count < rt0->rt_hops)))
    {
      // If we have a fresher seq no. or lesser #hops for the
      // same seq no., update the rt entry. Else don't bother.
      rt_update(rt0, rq->rq_src_seqno, rq->rq_hop_count, ih->saddr(),
                max(rt0->rt_expire, (CURRENT_TIME + REV_ROUTE_LIFE)));
      if (rt0->rt_req_timeout > 0.0)
      {
        
        rt0->rt_req_cnt = 0;
        rt0->rt_req_timeout = 0.0;
        rt0->rt_req_last_ttl = rq->rq_hop_count;
        rt0->rt_expire = CURRENT_TIME + ACTIVE_ROUTE_TIMEOUT;
      }
  
      /* Find out whether any buffered packet can benefit from the 
        * reverse route.
        * May need some change in the following code - Mahesh 09/11/99
        */
      assert(rt0->rt_flags == RTF_UP);
      Packet *buffered_pkt;
      while ((buffered_pkt = rqueue.deque(rt0->rt_dst)))
      {
        if (rt0 && (rt0->rt_flags == RTF_UP))
        {
          assert(rt0->rt_hops != INFINITY2);
          forward(rt0, buffered_pkt, NO_DELAY);
        }
      }
    }
    // End for putting reverse route in rt table
  
    /*
    * We have taken care of the reverse route stuff.
    * Now see whether we can send a route reply. 
    */

    rt = rtable.rt_lookup(rq->rq_dst);
  
    // First check if I am the destination ..
  
    if (rq->rq_dst == index)
    {
  
  #ifdef DEBUG
      fprintf(stderr, "%d - %s: destination sending reply\n",
              index, __FUNCTION__);
  #endif // DEBUG
  
      // Just to be safe, I use the max. Somebody may have
      // incremented the dst seqno.
      seqno = max(seqno, rq->rq_dst_seqno) + 1;
      if (seqno % 2)
        seqno++;
  
      sendReply(rq->rq_src,        // IP Destination
                1,                 // Hop Count
                index,             // Dest IP Address
                seqno,             // Dest Sequence Num
                MY_ROUTE_TIMEOUT,  // Lifetime
                rq->rq_timestamp); // timestamp
  
      Packet::free(p);
    }
  
    // I am not the destination, but I may have a fresh enough route.
  
    else if (rt && (rt->rt_hops != INFINITY2) &&
             (rt->rt_seqno >= rq->rq_dst_seqno))
    {
  
      //assert (rt->rt_flags == RTF_UP);
      assert(rq->rq_dst == rt->rt_dst);
      //assert ((rt->rt_seqno%2) == 0); // is the seqno even?
      sendReply(rq->rq_src,
                rt->rt_hops + 1,
                rq->rq_dst,
                rt->rt_seqno,
                (u_int32_t)(rt->rt_expire - CURRENT_TIME),
                //             rt->rt_expire - CURRENT_TIME,
                rq->rq_timestamp);
      // Insert nexthops to RREQ source and RREQ destination in the
      // precursor lists of destination and source respectively
      rt->pc_insert(rt0->rt_nexthop); // nexthop to RREQ source
      rt0->pc_insert(rt->rt_nexthop); // nexthop to RREQ destination
  
  #ifdef RREQ_GRAT_RREP
  
      sendReply(rq->rq_dst,
                rq->rq_hop_count,
                rq->rq_src,
                rq->rq_src_seqno,
                (u_int32_t)(rt->rt_expire - CURRENT_TIME),
                //             rt->rt_expire - CURRENT_TIME,
                rq->rq_timestamp);
  #endif
  
      // TODO: send grat RREP to dst if G flag set in RREQ using rq->rq_src_seqno, rq->rq_hop_counT
  
      // DONE: Included gratuitous replies to be sent as per IETF aodv draft specification. As of now, G flag has not been dynamically used and is always set or reset in aodv-packet.h --- Anant Utgikar, 09/16/02.
  
      Packet::free(p);
    }
    /*
    * Can't reply. So forward the  Route Request
    */
    else
    {
      ih->saddr() = index;
      ih->daddr() = IP_BROADCAST;
      rq->rq_hop_count += 1;
      // Maximum sequence number seen en route
      if (rt)
        rq->rq_dst_seqno = max(rt->rt_seqno, rq->rq_dst_seqno);
      forward((aodv_rt_entry *)0, p, DELAY);
    }
  // }
  
}

void AODV::recvReply(Packet *p)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::recvReply", now);
  fclose(fp);

  //struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_aodv_reply *rp = HDR_AODV_REPLY(p);
  aodv_rt_entry *rt;
  char suppress_reply = 0;
  double delay = 0.0;

#ifdef DEBUG
  fprintf(stderr, "%d - %s: received a REPLY\n", index, __FUNCTION__);
#endif // DEBUG

  /*
  *  Got a reply. So reset the "soft state" maintained for 
  *  route requests in the request table. We don't really have
  *  have a separate request table. It is just a part of the
  *  routing table itself. 
  */
  // Note that rp_dst is the dest of the data packets, not the
  // the dest of the reply, which is the src of the data packets.

  rt = rtable.rt_lookup(rp->rp_dst);

  /*
  *  If I don't have a rt entry to this host... adding
  */
  if (rt == 0)
  {
    rt = rtable.rt_add(rp->rp_dst);
  }

  /*
  * Add a forward route table entry... here I am following 
  * Perkins-Royer AODV paper almost literally - SRD 5/99
  */

  if ((rt->rt_seqno < rp->rp_dst_seqno) || // newer route
      ((rt->rt_seqno == rp->rp_dst_seqno) &&
       (rt->rt_hops > rp->rp_hop_count)))
  { // shorter or better route

    // Update the rt entry
    rt_update(rt, rp->rp_dst_seqno, rp->rp_hop_count,
              rp->rp_src, CURRENT_TIME + rp->rp_lifetime);

    // reset the soft state
    rt->rt_req_cnt = 0;
    rt->rt_req_timeout = 0.0;
    rt->rt_req_last_ttl = rp->rp_hop_count;

    if (ih->daddr() == index)
    { // If I am the original source
      // Update the route discovery latency statistics
      // rp->rp_timestamp is the time of request origination

      rt->rt_disc_latency[(unsigned char)rt->hist_indx] = (CURRENT_TIME - rp->rp_timestamp) / (double)rp->rp_hop_count;
      // increment indx for next time
      rt->hist_indx = (rt->hist_indx + 1) % MAX_HISTORY;
    }

    /*
   * Send all packets queued in the sendbuffer destined for
   * this destination. 
   * XXX - observe the "second" use of p.
   */
    Packet *buf_pkt;
    while ((buf_pkt = rqueue.deque(rt->rt_dst)))
    {
      if (rt->rt_hops != INFINITY2)
      {
        assert(rt->rt_flags == RTF_UP);
        // Delay them a little to help ARP. Otherwise ARP
        // may drop packets. -SRD 5/23/99
        forward(rt, buf_pkt, delay);
        delay += ARP_DELAY;
      }
    }
  }
  else
  {
    suppress_reply = 1;
  }

  /*
  * If reply is for me, discard it.
  */

  if (ih->daddr() == index || suppress_reply)
  {
    Packet::free(p);
  }
  /*
  * Otherwise, forward the Route Reply.
  */
  else
  {
    // Find the rt entry
    aodv_rt_entry *rt0 = rtable.rt_lookup(ih->daddr());
    // If the rt is up, forward
    if (rt0 && (rt0->rt_hops != INFINITY2))
    {
      assert(rt0->rt_flags == RTF_UP);
      rp->rp_hop_count += 1;
      rp->rp_src = index;
      forward(rt0, p, NO_DELAY);
      // Insert the nexthop towards the RREQ source to
      // the precursor list of the RREQ destination
      rt->pc_insert(rt0->rt_nexthop); // nexthop to RREQ source
    }
    else
    {
    // I don't know how to forward .. drop the reply.
#ifdef DEBUG
      fprintf(stderr, "%s: dropping Route Reply\n", __FUNCTION__);
#endif // DEBUG
      drop(p, DROP_RTR_NO_ROUTE);
    }
  }
}

void AODV::recvError(Packet *p)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::recvError", now);
  fclose(fp);

  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_aodv_error *re = HDR_AODV_ERROR(p);
  aodv_rt_entry *rt;
  u_int8_t i;
  Packet *rerr = Packet::alloc();
  struct hdr_aodv_error *nre = HDR_AODV_ERROR(rerr);

  nre->DestCount = 0;

  for (i = 0; i < re->DestCount; i++)
  {
    // For each unreachable destination
    rt = rtable.rt_lookup(re->unreachable_dst[i]);
    if (rt && (rt->rt_hops != INFINITY2) &&
        (rt->rt_nexthop == ih->saddr()) &&
        (rt->rt_seqno <= re->unreachable_dst_seqno[i]))
    {
      assert(rt->rt_flags == RTF_UP);
      assert((rt->rt_seqno % 2) == 0); // is the seqno even?
#ifdef DEBUG
      fprintf(stderr, "%s(%f): %d\t(%d\t%u\t%d)\t(%d\t%u\t%d)\n", __FUNCTION__, CURRENT_TIME,
              index, rt->rt_dst, rt->rt_seqno, rt->rt_nexthop,
              re->unreachable_dst[i], re->unreachable_dst_seqno[i],
              ih->saddr());
#endif // DEBUG
      rt->rt_seqno = re->unreachable_dst_seqno[i];
      rt_down(rt);

      // Not sure whether this is the right thing to do
      Packet *pkt;
      while ((pkt = ifqueue->filter(ih->saddr())))
      {
        drop(pkt, DROP_RTR_MAC_CALLBACK);
      }

      // if precursor list non-empty add to RERR and delete the precursor list
      if (!rt->pc_empty())
      {
        nre->unreachable_dst[nre->DestCount] = rt->rt_dst;
        nre->unreachable_dst_seqno[nre->DestCount] = rt->rt_seqno;
        nre->DestCount += 1;
        rt->pc_delete();
      }
    }
  }

  if (nre->DestCount > 0)
  {
#ifdef DEBUG
    fprintf(stderr, "%s(%f): %d\t sending RERR...\n", __FUNCTION__, CURRENT_TIME, index);
#endif // DEBUG
    sendError(rerr);
  }
  else
  {
    Packet::free(rerr);
  }

  Packet::free(p);
}

/*
   Packet Transmission Routines
*/

void AODV::forward(aodv_rt_entry *rt, Packet *p, double delay)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::forward", now);
  fclose(fp);

  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);

   #ifdef DEBUG && index > 0
    // manet
    iNode=    (MobileNode *) (Node::get_node_by_address (index) ); 
    xpos=     iNode->X(); 
    ypos=     iNode->Y(); 
    iEnergy=  iNode->energy_model()->energy();
    fp = fopen("debug.txt", "a");
    fprintf(fp, "\n%.6f  Posisi node %d adalah X =%.4f dan Y =%.4f",CURRENT_TIME, index, xpos, ypos); 
    fprintf(fp, "\n%.6f  Energi untuk node %d sejumlah %.4f",CURRENT_TIME, index, iEnergy); 
    fclose(fp);
    // fprintf(fp, "\n%.6f  Routing agent is initialized for node %d", CURRENT_TIME, index); 
    // fprintf(fp, "\n%.6f  Destination, NextHop, Hop, Seqno, expire, flag");
    // fprintf(fp, "\n%.6f  NODE: %i %i %i %i %i %.4lf %d", CURRENT_TIME, rt->rt_dst, rt->rt_nexthop, rt->rt_hops, rt->rt_seqno, rt->rt_expire, rt->rt_flags);
    // fprintf(fp, "\n%.6f  NODE: %d %d %d %d %d %.4f %d", CURRENT_TIME, rt->rt_dst, rt->rt_nexthop, rt->rt_hops, rt->rt_seqno, rt->rt_expire, rt->rt_flags); 
  #endif

  if (ih->ttl_ == 0)
  {

#ifdef DEBUG
    fprintf(stderr, "%s: calling drop()\n", __PRETTY_FUNCTION__);
#endif // DEBUG

    drop(p, DROP_RTR_TTL);
    return;
  }

  if (((ch->ptype() != PT_AODV && ch->direction() == hdr_cmn::UP) &&
       ((u_int32_t)ih->daddr() == IP_BROADCAST)) ||
      (ih->daddr() == here_.addr_))
  {
    dmux_->recv(p, 0);
    return;
  }

  if (rt)
  {
    assert(rt->rt_flags == RTF_UP);
    rt->rt_expire = CURRENT_TIME + ACTIVE_ROUTE_TIMEOUT;
    ch->next_hop_ = rt->rt_nexthop;
    ch->addr_type() = NS_AF_INET;
    ch->direction() = hdr_cmn::DOWN; //important: change the packet's direction
  }
  else
  { // if it is a broadcast packet
    // assert(ch->ptype() == PT_AODV); // maybe a diff pkt type like gaf
    assert(ih->daddr() == (nsaddr_t)IP_BROADCAST);
    ch->addr_type() = NS_AF_NONE;
    ch->direction() = hdr_cmn::DOWN; //important: change the packet's direction
  }

  if (ih->daddr() == (nsaddr_t)IP_BROADCAST)
  {
    // If it is a broadcast packet
    assert(rt == 0);
    if (ch->ptype() == PT_AODV)
    {
      /*
      *  Jitter the sending of AODV broadcast packets by 10ms
      */
      Scheduler::instance().schedule(target_, p,
                                     0.01 * Random::uniform());
    }
    else
    {
      Scheduler::instance().schedule(target_, p, 0.); // No jitter
    }
  }
  else
  { // Not a broadcast packet
    if (delay > 0.0)
    {
      Scheduler::instance().schedule(target_, p, delay);
    }
    else
    {
      // Not a broadcast packet, no delay, send immediately
      Scheduler::instance().schedule(target_, p, 0.);
    }
  }
}

void AODV::sendRequest(nsaddr_t dst)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::sendRequest sedang berada di node: %d", now, index);
  fclose(fp);

  // Allocate a RREQ packet
  Packet *p = Packet::alloc();
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_aodv_request *rq = HDR_AODV_REQUEST(p);
  aodv_rt_entry *rt = rtable.rt_lookup(dst);

  assert(rt);

  /*
  *  Rate limit sending of Route Requests. We are very conservative
  *  about sending out route requests. 
  */

  if (rt->rt_flags == RTF_UP)
  {
    assert(rt->rt_hops != INFINITY2);
    Packet::free((Packet *)p);
    return;
  }

  if (rt->rt_req_timeout > CURRENT_TIME)
  {
    Packet::free((Packet *)p);
    return;
  }

  // rt_req_cnt is the no. of times we did network-wide broadcast
  // RREQ_RETRIES is the maximum number we will allow before
  // going to a long timeout.

  if (rt->rt_req_cnt > RREQ_RETRIES)
  {
    rt->rt_req_timeout = CURRENT_TIME + MAX_RREQ_TIMEOUT;
    rt->rt_req_cnt = 0;
    Packet *buf_pkt;
    while ((buf_pkt = rqueue.deque(rt->rt_dst)))
    {
      drop(buf_pkt, DROP_RTR_NO_ROUTE);
    }
    Packet::free((Packet *)p);
    return;
  }

#ifdef DEBUG
  fprintf(stderr, "(%2d) - %2d sending Route Request, dst: %d\n",
          ++route_request, index, rt->rt_dst);
#endif // DEBUG

  // Determine the TTL to be used this time.
  // Dynamic TTL evaluation - SRD

  rt->rt_req_last_ttl = max(rt->rt_req_last_ttl, rt->rt_last_hop_count);

  if (0 == rt->rt_req_last_ttl)
  {
    // first time query broadcast
    ih->ttl_ = TTL_START;
  }
  else
  {
    // Expanding ring search.
    if (rt->rt_req_last_ttl < TTL_THRESHOLD)
      ih->ttl_ = rt->rt_req_last_ttl + TTL_INCREMENT;
    else
    {
      // network-wide broadcast
      ih->ttl_ = NETWORK_DIAMETER;
      rt->rt_req_cnt += 1;
    }
  }

  // remember the TTL used  for the next time
  rt->rt_req_last_ttl = ih->ttl_;

  // PerHopTime is the roundtrip time per hop for route requests.
  // The factor 2.0 is just to be safe .. SRD 5/22/99
  // Also note that we are making timeouts to be larger if we have
  // done network wide broadcast before.

  rt->rt_req_timeout = 2.0 * (double)ih->ttl_ * PerHopTime(rt);
  if (rt->rt_req_cnt > 0)
    rt->rt_req_timeout *= rt->rt_req_cnt;
  rt->rt_req_timeout += CURRENT_TIME;

  // Don't let the timeout to be too large, however .. SRD 6/8/99
  if (rt->rt_req_timeout > CURRENT_TIME + MAX_RREQ_TIMEOUT)
    rt->rt_req_timeout = CURRENT_TIME + MAX_RREQ_TIMEOUT;
  rt->rt_expire = 0;

#ifdef DEBUG
  fprintf(stderr, "(%2d) - %2d sending Route Request, dst: %d, tout %f ms\n",
          ++route_request,
          index, rt->rt_dst,
          rt->rt_req_timeout - CURRENT_TIME);
#endif // DEBUG

  // Fill out the RREQ packet
  // ch->uid() = 0;
  ch->ptype() = PT_AODV;
  ch->size() = IP_HDR_LEN + rq->size();
  ch->iface() = -2;
  ch->error() = 0;
  ch->addr_type() = NS_AF_NONE;
  ch->prev_hop_ = index; // AODV hack

  ih->saddr() = index;
  ih->daddr() = IP_BROADCAST;
  ih->sport() = RT_PORT;
  ih->dport() = RT_PORT;

  iNode=    (MobileNode *) (Node::get_node_by_address (index) ); 
  iEnergy=  iNode->energy_model()->energy();
  // Fill up some more fields.
  rq->rq_type = AODVTYPE_RREQ;
  rq->rq_hop_count = 1;
  rq->rq_bcast_id = bid++;
  rq->rq_dst = dst;
  rq->rq_dst_seqno = (rt ? rt->rt_seqno : 0);
  rq->rq_src = index;
  rq->rq_min_life=  iEnergy;
  seqno += 2;
  assert((seqno % 2) == 0);
  rq->rq_src_seqno = seqno;
  rq->rq_timestamp = CURRENT_TIME;

  int cluster_head = CH_ID; // modifikasi kandidat cluster head menjadi cluster head dan pemberian ID pada untuk broadcast

  if (CH_ID == -1) // Init cluster head belum ditemukan diawal, lakukan perhitungan
    
    calculateCHID();

  rq->rq_cluster_head_index = CH_ID;
  CH_ID = cluster_head;

  if (CH_ID != index) // modifikasi jika suatu node bukan CH, lakukan perhitungan.
    
    rq->rq_bcast_id = CH_ID;  // modfikasi CH sudah ditemukan, lakukan broadcast ke node lain bahwa node tsb CH

  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::sendRequest sedang berada di node: %d, rq_cluster_head_index: %d, CH_ID: %d", now, index, rq->rq_cluster_head_index, CH_ID);
  fclose(fp);

  Scheduler::instance().schedule(target_, p, 0.);
}

void AODV::sendReply(nsaddr_t ipdst, u_int32_t hop_count, nsaddr_t rpdst,
                     u_int32_t rpseq, u_int32_t lifetime, double timestamp)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::sendReply", now);
  fclose(fp);

  Packet *p = Packet::alloc();
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_aodv_reply *rp = HDR_AODV_REPLY(p);
  aodv_rt_entry *rt = rtable.rt_lookup(ipdst);

#ifdef DEBUG
  fprintf(stderr, "sending Reply from %d at %.2f\n", index, Scheduler::instance().clock());
#endif // DEBUG
  assert(rt);

  rp->rp_type = AODVTYPE_RREP;
  //rp->rp_flags = 0x00;
  rp->rp_hop_count = hop_count;
  rp->rp_dst = rpdst;
  rp->rp_dst_seqno = rpseq;
  rp->rp_src = index;
  rp->rp_lifetime = lifetime;
  rp->rp_timestamp = timestamp;

  // ch->uid() = 0;
  ch->ptype() = PT_AODV;
  ch->size() = IP_HDR_LEN + rp->size();
  ch->iface() = -2;
  ch->error() = 0;
  ch->addr_type() = NS_AF_INET;
  ch->next_hop_ = rt->rt_nexthop;
  ch->prev_hop_ = index; // AODV hack
  ch->direction() = hdr_cmn::DOWN;

  ih->saddr() = index;
  ih->daddr() = ipdst;
  ih->sport() = RT_PORT;
  ih->dport() = RT_PORT;
  ih->ttl_ = NETWORK_DIAMETER;

  Scheduler::instance().schedule(target_, p, 0.);
}

void AODV::sendError(Packet *p, bool jitter)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::sendError", now);
  fclose(fp);

  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_aodv_error *re = HDR_AODV_ERROR(p);

#ifdef ERROR
  fprintf(stderr, "sending Error from %d at %.2f\n", index, Scheduler::instance().clock());
#endif // DEBUG

  re->re_type = AODVTYPE_RERR;
  //re->reserved[0] = 0x00; re->reserved[1] = 0x00;
  // DestCount and list of unreachable destinations are already filled

  // ch->uid() = 0;
  ch->ptype() = PT_AODV;
  ch->size() = IP_HDR_LEN + re->size();
  ch->iface() = -2;
  ch->error() = 0;
  ch->addr_type() = NS_AF_NONE;
  ch->next_hop_ = 0;
  ch->prev_hop_ = index;           // 
  ch->direction() = hdr_cmn::DOWN; //important: change the packet's direction

  ih->saddr() = index;
  ih->daddr() = IP_BROADCAST;
  ih->sport() = RT_PORT;
  ih->dport() = RT_PORT;
  ih->ttl_ = 1;

  // Do we need any jitter? Yes
  if (jitter)
    Scheduler::instance().schedule(target_, p, 0.01 * Random::uniform());
  else
    Scheduler::instance().schedule(target_, p, 0.0);
}

/*
   Neighbor Management Functions
*/

void AODV::sendHello()
{
  if (nodes_count<=index) nodes_count++;
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::sendHello sedang berada di node: %d, jumlah node %d", now, index, nodes_count);
  fclose(fp);

  Packet *p = Packet::alloc();
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_ip *ih = HDR_IP(p);
  struct hdr_aodv_reply *rh = HDR_AODV_REPLY(p);

#ifdef DEBUG
  fprintf(stderr, "sending Hello from %d at %.2f\n", index, Scheduler::instance().clock());
#endif // DEBUG

  rh->rp_type = AODVTYPE_HELLO;
  //rh->rp_flags = 0x00;
  rh->rp_hop_count = 1;
  rh->rp_dst = index;
  rh->rp_dst_seqno = seqno;
  rh->rp_lifetime = (1 + ALLOWED_HELLO_LOSS) * HELLO_INTERVAL;

  // ch->uid() = 0;
  ch->ptype() = PT_AODV;
  ch->size() = IP_HDR_LEN + rh->size();
  ch->iface() = -2;
  ch->error() = 0;
  ch->addr_type() = NS_AF_NONE;
  ch->prev_hop_ = index; 

  ih->saddr() = index;
  ih->daddr() = IP_BROADCAST;
  ih->sport() = RT_PORT;
  ih->dport() = RT_PORT;
  ih->ttl_ = 1;             // modifikasi memberi pesan ke next node gateway

  Scheduler::instance().schedule(target_, p, 0.0);
}

void AODV::recvHello(Packet *p)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::recvHello sedang berada di node: %d", now, index);
  fclose(fp);

  //struct hdr_ip *ih = HDR_IP(p);
  struct hdr_aodv_reply *rp = HDR_AODV_REPLY(p);
  AODV_Neighbor *nb;

  // modifikasi menerima pesan dari node pengirim pesan dan node penerima pesan tentang isi tetangga suatu node
  Node *sender_node = Node::get_node_by_address(rp->rp_dst);
  Node *receiver_node = Node::get_node_by_address(index);

  nb = nb_lookup(rp->rp_dst);
  if (nb == 0)
  {
    
    sender_node->addNeighbor(receiver_node);
    receiver_node->addNeighbor(sender_node);

    nb_insert(rp->rp_dst);

    
    Node* m_node = Node::get_node_by_address(this->addr());
    neighbor_list_node* my_mobile_neighbor_list;
    my_mobile_neighbor_list = m_node->neighbor_list_;
    int count = 0;
    
    //cout<<"##Node id : " <<index<<endl;
    while(my_mobile_neighbor_list)
    {
        //cout<<"##Neighbor ID:"<<my_mobile_neighbor_list->nodeid<<endl;
        count++;
        if(my_mobile_neighbor_list->next){
            my_mobile_neighbor_list=my_mobile_neighbor_list->next;
        }
        else{
            //cout<<"## Jumlah: "<<count<<endl;
            //cout<<"## Jumlah: "<<nb_node()<<endl;
            break;
        }
    }
    //cout<<"##Jumlah: "<<count<<endl;
  }
  else
  {
    nb->nb_expire = CURRENT_TIME +
                    (1.5 * ALLOWED_HELLO_LOSS * HELLO_INTERVAL);
  }
  

  Packet::free(p);
}

void AODV::nb_insert(nsaddr_t id)
{
  count_neighbour[index]+=1;            // modifikasi penambahan node dan perhitungan tetangga
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::nb_insert sedang berada di node: %d menambah node: %d jumlah tetangga %d", now, index, id, count_neighbour[index]);
  fclose(fp);

  AODV_Neighbor *nb = new AODV_Neighbor(id);

  assert(nb);
  nb->nb_expire = CURRENT_TIME +
                  (1.5 * ALLOWED_HELLO_LOSS * HELLO_INTERVAL);
  LIST_INSERT_HEAD(&nbhead, nb, nb_link);
  seqno += 2; // set dari tetangga yang berubah
  assert((seqno % 2) == 0);
}

AODV_Neighbor *
AODV::nb_lookup(nsaddr_t id)
{
  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::nb_lookup sedang berada di node: %d", now, index);
  fclose(fp);

  AODV_Neighbor *nb = nbhead.lh_first;

  for (; nb; nb = nb->nb_link.le_next)
  {
    if (nb->nb_addr == id)
      break;
  }
  return nb;
}

/*
 * Called when we receive *explicit* notification that a Neighbor
 * is no longer reachable.
 */
void AODV::nb_delete(nsaddr_t id)
{
  count_neighbour[index]-=1;        

  //print fungsi
  double now = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::nb_delete sedang berada di node: %d menghapus node: %d jumlah tetangga %d", now, index, id, count_neighbour[index]);
  fclose(fp);

  AODV_Neighbor *nb = nbhead.lh_first;

  log_link_del(id);
  seqno += 2; // Set of neighbors changed
  assert((seqno % 2) == 0);

  for (; nb; nb = nb->nb_link.le_next)
  {
    if (nb->nb_addr == id)
    {
      LIST_REMOVE(nb, nb_link);
      delete nb;
      break;
    }
  }

  handle_link_failure(id);
}

/*
 * Purges all timed-out Neighbor Entries - runs every
 * HELLO_INTERVAL * 1.5 seconds.
 */
void AODV::nb_purge()
{

  //print fungsi
  double nows = Scheduler::instance().clock(); // get the time
  FILE *fp;
  fp = fopen("debug.txt", "a");
  fprintf(fp, "\n %f fungsi AODV::nb_purge", nows);
  fclose(fp);

  AODV_Neighbor *nb = nbhead.lh_first;
  AODV_Neighbor *nbn;
  double now = CURRENT_TIME;

  for (; nb; nb = nbn)
  {
    nbn = nb->nb_link.le_next;
    if (nb->nb_expire <= now)
    {
      nb_delete(nb->nb_addr);
    }
  }
}