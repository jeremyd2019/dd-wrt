
/***************************************************************************
 * PacketStats.cc -- The PacketStats class handles packet statistics. It   *
 * is intended to keep track of the number of packets and bytes sent and   *
 * received, keep track of start and finish times, etc.                    *
 *                                                                         *
 ***********************IMPORTANT NMAP LICENSE TERMS************************
 *                                                                         *
 * The Nmap Security Scanner is (C) 1996-2012 Insecure.Com LLC. Nmap is    *
 * also a registered trademark of Insecure.Com LLC.  This program is free  *
 * software; you may redistribute and/or modify it under the terms of the  *
 * GNU General Public License as published by the Free Software            *
 * Foundation; Version 2 with the clarifications and exceptions described  *
 * below.  This guarantees your right to use, modify, and redistribute     *
 * this software under certain conditions.  If you wish to embed Nmap      *
 * technology into proprietary software, we sell alternative licenses      *
 * (contact sales@insecure.com).  Dozens of software vendors already       *
 * license Nmap technology such as host discovery, port scanning, OS       *
 * detection, version detection, and the Nmap Scripting Engine.            *
 *                                                                         *
 * Note that the GPL places important restrictions on "derived works", yet *
 * it does not provide a detailed definition of that term.  To avoid       *
 * misunderstandings, we interpret that term as broadly as copyright law   *
 * allows.  For example, we consider an application to constitute a        *
 * "derivative work" for the purpose of this license if it does any of the *
 * following:                                                              *
 * o Integrates source code from Nmap                                      *
 * o Reads or includes Nmap copyrighted data files, such as                *
 *   nmap-os-db or nmap-service-probes.                                    *
 * o Executes Nmap and parses the results (as opposed to typical shell or  *
 *   execution-menu apps, which simply display raw Nmap output and so are  *
 *   not derivative works.)                                                *
 * o Integrates/includes/aggregates Nmap into a proprietary executable     *
 *   installer, such as those produced by InstallShield.                   *
 * o Links to a library or executes a program that does any of the above   *
 *                                                                         *
 * The term "Nmap" should be taken to also include any portions or derived *
 * works of Nmap, as well as other software we distribute under this       *
 * license such as Zenmap, Ncat, and Nping.  This list is not exclusive,   *
 * but is meant to clarify our interpretation of derived works with some   *
 * common examples.  Our interpretation applies only to Nmap--we don't     *
 * speak for other people's GPL works.                                     *
 *                                                                         *
 * If you have any questions about the GPL licensing restrictions on using *
 * Nmap in non-GPL works, we would be happy to help.  As mentioned above,  *
 * we also offer alternative license to integrate Nmap into proprietary    *
 * applications and appliances.  These contracts have been sold to dozens  *
 * of software vendors, and generally include a perpetual license as well  *
 * as providing for priority support and updates.  They also fund the      *
 * continued development of Nmap.  Please email sales@insecure.com for     *
 * further information.                                                    *
 *                                                                         *
 * As a special exception to the GPL terms, Insecure.Com LLC grants        *
 * permission to link the code of this program with any version of the     *
 * OpenSSL library which is distributed under a license identical to that  *
 * listed in the included docs/licenses/OpenSSL.txt file, and distribute   *
 * linked combinations including the two. You must obey the GNU GPL in all *
 * respects for all of the code used other than OpenSSL.  If you modify    *
 * this file, you may extend this exception to your version of the file,   *
 * but you are not obligated to do so.                                     *
 *                                                                         *
 * If you received these files with a written license agreement or         *
 * contract stating terms other than the terms above, then that            *
 * alternative license agreement takes precedence over these comments.     *
 *                                                                         *
 * Source is provided to this software because we believe users have a     *
 * right to know exactly what a program is going to do before they run it. *
 * This also allows you to audit the software for security holes (none     *
 * have been found so far).                                                *
 *                                                                         *
 * Source code also allows you to port Nmap to new platforms, fix bugs,    *
 * and add new features.  You are highly encouraged to send your changes   *
 * to nmap-dev@insecure.org for possible incorporation into the main       *
 * distribution.  By sending these changes to Fyodor or one of the         *
 * Insecure.Org development mailing lists, or checking them into the Nmap  *
 * source code repository, it is understood (unless you specify otherwise) *
 * that you are offering the Nmap Project (Insecure.Com LLC) the           *
 * unlimited, non-exclusive right to reuse, modify, and relicense the      *
 * code.  Nmap will always be available Open Source, but this is important *
 * because the inability to relicense code has caused devastating problems *
 * for other Free Software projects (such as KDE and NASM).  We also       *
 * occasionally relicense the code to third parties as discussed above.    *
 * If you wish to specify special license conditions of your               *
 * contributions, just say so when you send them.                          *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU       *
 * General Public License v2.0 for more details at                         *
 * http://www.gnu.org/licenses/gpl-2.0.html , or in the COPYING file       *
 * included with Nmap.                                                     *
 *                                                                         *
 ***************************************************************************/

#include "nping.h"
#include "stats.h"
#include "NpingOps.h"
#include "output.h"



/*****************************************************************************/
/* Implementation of NpingTimer class.                                       */
/*****************************************************************************/

NpingTimer::NpingTimer(){
  this->reset();
}

NpingTimer::~NpingTimer(){

}

void NpingTimer::reset(){
  this->start_tv.tv_sec=0;
  this->start_tv.tv_usec=0;
  this->stop_tv.tv_sec=0;
  this->stop_tv.tv_usec=0;
}


int NpingTimer::start(){
  if( timeval_set(&start_tv) || timeval_set(&stop_tv) )
    return OP_FAILURE;
  gettimeofday(&start_tv, NULL);
  return OP_SUCCESS;
}


int NpingTimer::stop(){
  if( !timeval_set(&start_tv) || timeval_set(&stop_tv) )
    return OP_FAILURE;
  gettimeofday(&stop_tv, NULL);
  return OP_SUCCESS;
}


double NpingTimer::elapsed(struct timeval *now){
  struct timeval tv;
  const struct timeval *end_tv=NULL;
  /* If for some reason the clock has not been started, 
   * just return 0 seconds elapsed. */
  if(!timeval_set(&start_tv)){
    return 0.0;
  }
  /* If the caller supplied a time, use it */
  if(now!=NULL){
    end_tv=now;
  /* If the clock has been stopped already, use the stop time */
  }else if(timeval_set(&stop_tv)){
    end_tv = &stop_tv;
  }else{
    gettimeofday(&tv, NULL);
    end_tv = &tv;
  }
  return TIMEVAL_SUBTRACT(*end_tv, start_tv) / 1000000.0;
}


bool NpingTimer::is_started(){
  return timeval_set(&this->start_tv);
}


bool NpingTimer::is_stopped(){
  return timeval_set(&this->stop_tv);
}


/* Returns true if tv has been initialized; i.e., its members are not all zero. */
bool NpingTimer::timeval_set(const struct timeval *tv) {
  return (tv->tv_sec != 0 || tv->tv_usec != 0);
}



/*****************************************************************************/
/* Implementation of NpingStats class.                                       */
/*****************************************************************************/

NpingStats::NpingStats(){
  this->reset();
}


NpingStats::~NpingStats(){

}


void NpingStats::reset(){
  this->packets_sent=0;
  this->packets_received=0;
  this->packets_echoed=0;

  this->bytes_sent=0;
  this->bytes_received=0;
  this->bytes_echoed=0;

  this->echo_clients_served=0;

  this->tx_timer.reset();
  this->rx_timer.reset();
  this->run_timer.reset();

} /* End of reset() */


/** Updates packet and byte count for transmitted packets. */
int NpingStats::addSentPacket(u32 len){
  this->packets_sent++;
  this->bytes_sent+=len;
  return OP_SUCCESS;
} /* End of addSentPacket() */


/** Updates packet and byte count for received packets. */
int NpingStats::addRecvPacket(u32 len){
  this->packets_received++;
  this->bytes_received+=len;
  return OP_SUCCESS;
} /* End of addRecvPacket() */


/** Updates packet and byte count for echoed packets. */
int NpingStats::addEchoedPacket(u32 len){
  this->packets_echoed++;
  this->bytes_echoed+=len;
  return OP_SUCCESS;
} /* End of addEchoedPacket() */


/** Updates count for echo clients served by the echo server. */
int NpingStats::addEchoClientServed(){
  this->echo_clients_served++;
  return OP_SUCCESS;
} /* End of addEchoClientServed() */


int NpingStats::startClocks(){
  this->startTxClock();
  this->startRxClock();
  return OP_SUCCESS;
}


int NpingStats::stopClocks(){
  this->stopTxClock();
  this->stopRxClock();
  return OP_SUCCESS;
}


int NpingStats::startTxClock(){
  this->tx_timer.start();
  return OP_SUCCESS;
}


int NpingStats::stopTxClock(){
  this->tx_timer.stop();
  return OP_SUCCESS;
}

int NpingStats::startRxClock(){
  this->rx_timer.start();
  return OP_SUCCESS;
}


int NpingStats::stopRxClock(){
  this->rx_timer.stop();
  return OP_SUCCESS;
}


int NpingStats::startRuntime(){
  this->run_timer.start();
  return OP_SUCCESS;
}


int NpingStats::stopRuntime(){
  this->run_timer.start();
  return OP_SUCCESS;
}


double NpingStats::elapsedTx(){
  return this->tx_timer.elapsed();
}


double NpingStats::elapsedRx(){
  return this->rx_timer.elapsed();
}


double NpingStats::elapsedRuntime(struct timeval *now){
  return this->run_timer.elapsed(now);
}


u64_t NpingStats::getSentPackets(){
  return this->packets_sent;
} /* End of getSentPackets() */


u64_t NpingStats::getSentBytes(){
  return this->bytes_sent;
} /* End of getSentBytes() */


u64_t NpingStats::getRecvPackets(){
  return this->packets_received;
} /* End of getRecvPackets() */


u64_t NpingStats::getRecvBytes(){
  return this->bytes_received;
} /* End of getRecvBytes() */


u64_t NpingStats::getEchoedPackets(){
  return this->packets_echoed;
} /* End of getEchoedPackets() */


u64_t NpingStats::getEchoedBytes(){
  return this->bytes_echoed;
} /* End of getEchoedBytes() */

u32 NpingStats::getEchoClientsServed(){
  return this->echo_clients_served;
} /* End of getEchoClientsServed() */


u64_t NpingStats::getLostPackets(){
  if(this->packets_sent <= this->packets_received)
    return 0;
  else
    return this->packets_sent - this->packets_received;
} /* End of getLostPackets() */


double NpingStats::getLostPacketPercentage(){
  u32 pkt_rcvd=this->packets_received;
  u32 pkt_sent=this->packets_sent;
  u32 pkt_lost=(pkt_rcvd>=pkt_sent) ? 0 : (u32)(pkt_sent-pkt_rcvd);
  /* Only compute percentage if we actually sent packets, don't do divisions
   * by zero! (this could happen when user presses CTRL-C and we print the
   * stats */
  double percentlost=0.0;
  if( pkt_lost!=0 && pkt_sent!=0)
    percentlost=((double)pkt_lost)/((double)pkt_sent);
  return percentlost;
} /* End of getLostPacketPercentage() */


double NpingStats::getLostPacketPercentage100(){
  return this->getLostPacketPercentage()*100;
} /* End of getLostPacketPercentage100() */


u64_t NpingStats::getUnmatchedPackets(){
  if(this->packets_received <= this->packets_echoed)
    return 0;
  else
    return this->packets_received - this->packets_echoed;
} /* End of getUnmatchedPackets() */


double NpingStats::getUnmatchedPacketPercentage(){
  u32 pkt_captured=this->packets_received;
  u32 pkt_echoed=this->packets_echoed;
  u32 pkt_unmatched=(pkt_captured<=pkt_echoed) ? 0 : (u32)(pkt_captured-pkt_echoed);
  double percentunmatched=0.0;
  if( pkt_unmatched!=0 && pkt_captured!=0)
    percentunmatched=((double)pkt_unmatched)/((double)pkt_captured);
  return percentunmatched;
} /* End of getUnmatchedPacketPercentage() */


double NpingStats::getUnmatchedPacketPercentage100(){
  return this->getUnmatchedPacketPercentage()*100;
} /* End of getUnmatchedPacketPercentage100() */


double NpingStats::getOverallTxPacketRate(){
  double elapsed = this->tx_timer.elapsed();
  if(elapsed <= 0.0)
    return 0.0;
  else
    return this->packets_sent / elapsed;
}


double NpingStats::getOverallTxByteRate(){
  double elapsed = this->tx_timer.elapsed();
  if(elapsed <= 0.0)
    return 0.0;
  else
    return this->bytes_sent / elapsed;
}


double NpingStats::getOverallRxPacketRate(){
  double elapsed = this->rx_timer.elapsed();
  if(elapsed <= 0.0)
    return 0.0;
  else
    return this->packets_received / elapsed;
}


double NpingStats::getOverallRxByteRate(){
  double elapsed = this->rx_timer.elapsed();
  if(elapsed <= 0.0)
    return 0.0;
  else
    return this->bytes_received / elapsed;
}

