#ifndef _CHORD_H_
#define _CHORD_H_

/*
 *
 * Copyright (C) 2000 Frans Kaashoek (kaashoek@lcs.mit.edu)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#include "sfsmisc.h"
#include "arpc.h"
#include "crypt.h"
#include "sys/time.h"
#include "vec.h"
#include "qhash.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#include "cache.h"
#include "chord_prot.h"
#include "chord_util.h"
#include "location.h"

#define NBIT     160     // size of Chord identifiers in bits
#define NSUCC    2*10     // 2 * log of # vnodes

typedef int cb_ID;

class vnode;

typedef vec<chordID> route;
typedef callback<void,chordID,bool,chordstat>::ref cbchallengeID_t;
typedef callback<void,vnode*>::ref cbjoin_t;
typedef callback<void,chordID,net_address,chordstat>::ref cbchordID_t;
typedef callback<void,chordID,route,chordstat>::ref cbroute_t;
typedef unsigned long cxid_t;
typedef callback<void, unsigned long, chord_RPC_arg *, cxid_t>::ref cbdispatch_t;

#define ACT_NODE_JOIN 1
#define ACT_NODE_UPDATE 2
#define ACT_NODE_LEAVE 3

struct findpredecessor_cbstate {
  chordID x;
  chordID nprime;
  route search_path;
  cbroute_t cb;
  findpredecessor_cbstate (chordID xi, chordID npi, route spi, cbroute_t cbi) :
    x (xi), nprime (npi), search_path (spi), cb (cbi) {};
};

struct finger {
  chordID start;
  node first; // first ID after start
};

class vnode : public virtual refcount {
  static const int stabilize_timer = 1000;  // millseconds
  static const int stabilize_timer_max = 500;      // seconds
  static const int max_retry = 5;
  
  ptr<locationtable> locations;
  finger finger_table[NBIT+1];
  node succlist[NSUCC+1];
  int nsucc;
  node predecessor;
  int myindex;
  bool stable;
  bool stable_fingers;
  bool stable_fingers2;
  bool stable_succlist;
  bool stable_succlist2;

  qhash<unsigned long, cbdispatch_t> dispatch_table;

  u_long nnodes;	  // estimate of the number of chord nodes
  u_long ngetsuccessor;
  u_long ngetpredecessor;
  u_long nfindsuccessor;
  u_long nhops;
  u_long nmaxhops;
  u_long nfindpredecessor;
  u_long nfindsuccessorrestart;
  u_long nfindpredecessorrestart;
  u_long ntestrange;
  u_long nnotify;
  u_long nalert;
  u_long ngetfingers;
  u_long nchallenge;

  u_long ndogetsuccessor;
  u_long ndogetpredecessor;
  u_long ndofindclosestpred;
  u_long ndonotify;
  u_long ndoalert;
  u_long ndotestrange;
  u_long ndogetfingers;
  u_long ndochallenge;

  timecb_t *stabilize_continuous_tmo;
  timecb_t *stabilize_backoff_tmo;
  u_int32_t continuous_timer;
  u_int32_t backoff_timer;

  void checkfingers (void);
  void updatefingers (chordID &x, net_address &r);
  void replacefinger (chordID &s, node *n);
  u_long estimate_nnodes ();
  chordID findpredfinger (chordID &x);
  chordID findpredfinger2 (chordID &x);

  u_int nout_backoff;
  u_int nout_continuous;
  void stabilize_backoff (int f, int s, u_int32_t t);
  void stabilize_continuous (u_int32_t t);
  int stabilize_succlist (int s);
  int stabilize_finger (int f);
  void stabilize_succ (void);
  void stabilize_pred (void);
  void stabilize_getpred_cb (chordID s, net_address r, chordstat status);
  void stabilize_getpred_cb_ok (chordID p, bool ok, chordstat status);
  void stabilize_findsucc_ok (int i, chordID s, bool ok, chordstat status);
  void stabilize_findsucc_cb (int i, chordID s, route path, chordstat status);
  void stabilize_getsucc_cb (chordID s, net_address r, chordstat status);
  void stabilize_getsucclist_cb (int i, chordID s, net_address r, 
				 chordstat status);
  void stabilize_getsucclist_ok (int i, chordID s, bool ok, chordstat status);
  void join_getsucc_ok (cbjoin_t cb, chordID s, bool ok, chordstat status);
  void join_getsucc_cb (cbjoin_t cb, chordID s, route r, chordstat status);
  void find_closestpred (chordID &n, chordID &x, findpredecessor_cbstate *st);
  void find_closestpred_cb (chordID n, findpredecessor_cbstate *st,
			    chord_noderes *res, clnt_stat err);
  void find_successor_cb (cbroute_t cb, route sp, chordID s, net_address r,
			    chordstat status);
  void find_predecessor_cb (cbroute_t cb, chordID x, chordID p, 
			    route search_path, chordstat status);
  void testrange_findclosestpred (chordID node, chordID x, 
				  findpredecessor_cbstate *st);
  void testrange_findclosestpred_cb (chord_testandfindres *res, 
			 findpredecessor_cbstate *st, clnt_stat err);
  void find_closestpred_succ_cb (findpredecessor_cbstate *st, chordID s,
				 net_address r, chordstat status);
  void get_successor_cb (chordID n, cbchordID_t cb, chord_noderes *res, 
			 clnt_stat err);
  void get_succ_cb (callback<void, chordID, chordstat>::ref cb, 
		    chordID succ, net_address r, chordstat err);
  void get_predecessor_cb (chordID n, cbchordID_t cb, chord_noderes *res, 
			 clnt_stat err);
  void donotify_cb (chordID p, bool ok, chordstat status);
  void notify_cb (chordID n, chordstat *res, clnt_stat err);
  void alert_cb (chordstat *res, clnt_stat err);
  void get_fingers (chordID &x);
  void get_fingers_cb (chordID x, chord_getfingersres *res, clnt_stat err);
  void challenge (chordID &x, cbchallengeID_t cb);
  void challenge_cb (int challenge, chordID x, cbchallengeID_t cb, 
		     chord_challengeres *res, clnt_stat err);
  void dofindsucc_cb (cbroute_t cb, chordID n, chordID x,
		      route search_path, chordstat status);
  void doalert_cb (svccb *sbp, chordID x, chordID s, net_address r, 
		   chordstat stat);
  void doRPC (chordID &ID, rpc_program prog, int procno, 
	      ptr<void> in, void *out, aclnt_cb cb);
 public:
  chordID myID;
  ptr<chord> chordnode;

  vnode (ptr<locationtable> _locations, ptr<chord> _chordnode, chordID _myID,
	 int _vnode);
  ~vnode (void);
  chordID my_ID () { return myID; };
  chordID my_pred () { return predecessor.n; };
  chordID my_succ ();

  // The API
  void stabilize (void);
  void join (cbjoin_t cb);
  void find_predecessor (chordID &n, chordID &x, cbroute_t cb);
  void find_predecessor_restart (chordID &n, chordID &x, route search_path,
				 cbroute_t cb);
  void find_successor (chordID &n, chordID &x, cbroute_t cb);
  void find_successor_restart (chordID &n, chordID &x, route search_path, 
			       cbroute_t cb);
  void get_successor (chordID n, cbchordID_t cb);
  void get_succ (chordID n, callback<void, chordID, chordstat>::ref cb);
  void get_predecessor (chordID n, cbchordID_t cb);
  void notify (chordID &n, chordID &x);
  void alert (chordID &n, chordID &x);
  void dofindsucc (chordID &n, cbroute_t cb);
  chordID nth_successorID (int n);

  // The RPCs
  void doget_successor (svccb *sbp);
  void doget_predecessor (svccb *sbp);
  void dofindclosestsucc (svccb *sbp, chord_findarg *fa);  
  void dofindclosestpred (svccb *sbp, chord_findarg *fa);
  void dotestrange_findclosestpred (svccb *sbp, chord_testandfindarg *fa);
  void donotify (svccb *sbp, chord_nodearg *na);
  void doalert (svccb *sbp, chord_nodearg *na);
  void dogetfingers (svccb *sbp);
  void dochallenge (svccb *sbp, chord_challengearg *ca);

  // For other modules
  int countrefs (chordID &x);
  chordID findsuccfinger (chordID &x);
  void deletefingers (chordID &x);
  void stats (void);
  void print (void);
  void stop (void);
  bool hasbecomeunstable (void);
  bool isstable (void);
  chordID lookup_closestpred (chordID &x);
  chordID lookup_closestsucc (chordID &x);

  //RPC demux
  void addHandler (unsigned long prog, cbdispatch_t cb) {
    dispatch_table.insert (prog, cb);
  };
  cbdispatch_t getHandler (unsigned long prog) {
    return dispatch_table [prog];
  };
};


class chord : public virtual refcount {
  int nvnode;
  net_address wellknownhost;
  int myport;
  chordID wellknownID;
  qhash<chordID, ref<vnode>, hashID> vnodes;
  ptr<vnode> active;

  void dispatch (ptr<asrv> s, ptr<axprt_dgram> x, svccb *sbp);
  int startchord (int myp);
  void deletefingers_cb (chordID x, const chordID &k, ptr<vnode> v);
  void stats_cb (const chordID &k, ptr<vnode> v);
  void print_cb (const chordID &k, ptr<vnode> v);
  void stop_cb (const chordID &k, ptr<vnode> v);
 
 public:
  // locations contains all nodes that appear as fingers in vnodes plus
  // a number of cached nodes.  the cached nodes have refcnt = 0
  ptr<locationtable> locations; 
    
  chord (str _wellknownhost, int _wellknownport, const chordID &_wellknownID,
	 int port, int set_rpcdelay, int max_cache, 
	 int max_connections);
  ptr<vnode> newvnode (cbjoin_t cb);
  void deletefingers (chordID x);
  int countrefs (chordID &x);
  void stats (void);
  void print (void);
  void stop (void);

  //support for demultiplexing RPCs to vnodes
  void register_handler (int progno, chordID dest, cbdispatch_t hand);

  //'wrappers' for vnode functions (to be called from higher layers)
  void set_active (int n) { 
    int i=0;
    qhash_slot<chordID, ref<vnode> > *s = vnodes.first ();
    while ( (s) && (i++ < n)) s = vnodes.next (s);
    if (!s) active = vnodes.first ()->value;
    else  active = s->value;

    warn << "Active node now " << active->my_ID () << "\n";
  };

  chordID lookup_closestpred (chordID k) { 
    return active->lookup_closestpred (k); 
  };
  void find_successor (chordID n, cbroute_t cb) {
    active->dofindsucc (n, cb);
  };
  void get_predecessor (chordID n, cbchordID_t cb) {
    active->get_predecessor (n, cb);
  };
  void doRPC (chordID &from, chordID &n, rpc_program progno, int procno, ptr<void> in, 
	      void *out, aclnt_cb cb) {
    locations->doRPC (from,  n , progno, procno, in, out, cb, getusec ());
  };
  void alert (chordID &n, chordID &x) {
    active->alert (n, x);
  };
  chordID nth_successorID (int n) {
    return active->nth_successorID (n);
  };
  chordID clnt_ID () {
    return active->my_ID ();
  };

  //public stats
  u_int64_t nrcv;
    
};

extern ptr<chord> chordnode;

#endif _CHORD_H_



