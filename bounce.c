/* $Id$
 * (c) 2000-2002 IC&S, The Netherlands
 *
 * Bounce.c implements functions to bounce email back to a sender 
 * with a message saying why the message was bounced */

	
#include "bounce.h"
#include "list.h"
#include "mime.h"
#include "db.h"
#include "debug.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>

extern struct list mimelist;  
extern struct list users;  
extern struct list smtpItems;  

int bounce (char *header, unsigned long headersize,char *destination_address, int type)
{
  void *sendmail_stream;
  struct list from_addresses;
  struct element *tmpelement;
  field_t dbmail_from_address, sendmail, postmaster;


  /* reading configuration from db */
  GetConfigValue("DBMAIL_FROM_ADDRESS", &smtpItems, dbmail_from_address);
  if (dbmail_from_address[0] == '\0')
    trace(TRACE_FATAL, "bounce(): DBMAIL_FROM_ADDRESS not configured (see config file). Stop.");

  GetConfigValue("SENDMAIL", &smtpItems, sendmail);
  if (sendmail[0] == '\0')
    trace(TRACE_FATAL, "bounce(): SENDMAIL not configured (see config file). Stop.");
	
  GetConfigValue("POSTMASTER", &smtpItems, postmaster);
  if (postmaster[0] == '\0')
    trace(TRACE_FATAL, "bounce(): POSTMASTER not configured (see config file). Stop.");
	
  
  trace (TRACE_DEBUG,"bounce(): creating bounce message for bounce type [%d]",type);
		
  if (!destination_address)
    {
      trace(TRACE_ERROR,"bounce(): cannot deliver to NULL.");
      return -1;
    }


  switch (type)
    {
    case BOUNCE_NO_SUCH_USER:
      /* no such user found */
      trace (TRACE_MESSAGE,"bounce(): sending 'no such user' bounce for destination [%s]",
	     destination_address);
      list_init(&from_addresses);
      /* scan the from header for addresses */
      mail_adr_list ("Return-Path", &from_addresses,&mimelist);

      if (list_totalnodes(&from_addresses) == 0)
	{
	  trace (TRACE_INFO,"bounce(): can't find Return-Path values, resorting to From values");
	  mail_adr_list ("From", &from_addresses, &mimelist);
	} /* RR logix :) */

      /* loop target addresses */
      if (list_totalnodes(&from_addresses) > 0)
	{
	  tmpelement=list_getstart (&from_addresses);
	  while (tmpelement!=NULL)
	    {
	      /* open a stream to sendmail 
		 the sendmail macro is defined in bounce.h */
	      
	      (FILE *)sendmail_stream=popen (sendmail,"w");

	      if (sendmail_stream==NULL)
		{
		  /* could not open a succesfull stream */
		  trace(TRACE_MESSAGE,"bounce(): could not open a pipe to %s",sendmail);
		  return -1;
		}
	      fprintf ((FILE *)sendmail_stream,"From: %s\n",dbmail_from_address);
	      fprintf ((FILE *)sendmail_stream,"To: %s\n",(char *)tmpelement->data);
	      fprintf ((FILE *)sendmail_stream,"Subject: DBMAIL: delivery failure\n");
	      fprintf ((FILE *)sendmail_stream,"\n");
	      fprintf ((FILE *)sendmail_stream,"This is the DBMAIL-SMTP program.\n\n");
	      fprintf ((FILE *)sendmail_stream,"I'm sorry to inform you that your message, addressed to %s,\n",
		       destination_address);
	      fprintf ((FILE *)sendmail_stream,"could not be delivered due to the following error.\n\n");
	      fprintf ((FILE *)sendmail_stream,"*** E-mail address %s is not known here. ***\n\n",destination_address);
	      fprintf ((FILE *)sendmail_stream,"If you think this message is incorrect please contact %s.\n\n",postmaster);
	      fprintf ((FILE *)sendmail_stream,"Header of your message follows...\n\n\n");
	      fprintf ((FILE *)sendmail_stream,"--- header of your message ---\n");
	      fprintf ((FILE *)sendmail_stream,"%s",header);
	      fprintf ((FILE *)sendmail_stream,"--- end of header ---\n\n\n");
	      fprintf ((FILE *)sendmail_stream,"\n.\n");
	      pclose ((FILE *)sendmail_stream);

	      /* jump forward to next recipient */
	      tmpelement=tmpelement->nextnode;
	    }
	}
      else
	{
	  trace(TRACE_MESSAGE,"bounce(): "
		"Message does not have a Return-Path nor a From headerfield, bounce failed");
	}
      
      break;
    case BOUNCE_STORAGE_LIMIT_REACHED:
      /* mailbox size exceeded */
      trace (TRACE_MESSAGE,"bounce(): sending 'mailboxsize exceeded' bounce for user [%s]",
	     destination_address);
      list_init(&from_addresses);
	
      /* scan the Return-Path header for addresses 
	 if they don't exist, resort to From addresses */
      mail_adr_list ("Return-Path", &from_addresses,&mimelist);
      
      if (list_totalnodes(&from_addresses) == 0)
	{
	  trace (TRACE_INFO,"bounce(): can't find Return-Path values, resorting to From values");
	  mail_adr_list ("From", &from_addresses, &mimelist);
	} /* RR logix :) */
      
      if (list_totalnodes(&from_addresses)>0)
	{
	  /* loop target addresses */
	  tmpelement=list_getstart (&from_addresses);
	  while (tmpelement!=NULL)
	    {
	      /* open a stream to sendmail 
		 the sendmail macro is defined in bounce.h */

	      (FILE *)sendmail_stream=popen (sendmail,"w");

	      if (sendmail_stream==NULL)
		{
		  /* could not open a succesfull stream */
		  trace(TRACE_MESSAGE,"bounce(): could not open a pipe to %s",sendmail);
		  return -1;
		}
	      fprintf ((FILE *)sendmail_stream,"From: %s\n",dbmail_from_address);
	      fprintf ((FILE *)sendmail_stream,"To: %s\n",(char *)tmpelement->data);
	      fprintf ((FILE *)sendmail_stream,"Subject: DBMAIL: delivery failure\n");
	      fprintf ((FILE *)sendmail_stream,"\n");
	      fprintf ((FILE *)sendmail_stream,"This is the DBMAIL-SMTP program.\n\n");
	      fprintf ((FILE *)sendmail_stream,"I'm sorry to inform you that your message, addressed to %s,\n",
		       destination_address);
	      fprintf ((FILE *)sendmail_stream,"could not be delivered due to the following error.\n\n");
	      fprintf ((FILE *)sendmail_stream,"*** Mailbox of user %s is FULL ***\n\n",destination_address);
	      fprintf ((FILE *)sendmail_stream,"If you think this message is incorrect please contact %s.\n\n",postmaster);
	      fprintf ((FILE *)sendmail_stream,"Header of your message follows...\n\n\n");
	      fprintf ((FILE *)sendmail_stream,"--- header of your message ---\n");
	      fprintf ((FILE *)sendmail_stream,"%s",header);
	      fprintf ((FILE *)sendmail_stream,"--- end of header ---\n\n\n");
	      fprintf ((FILE *)sendmail_stream,"\n.\n");
	      pclose ((FILE *)sendmail_stream);

	      /* jump forward to next recipient */
	      tmpelement=tmpelement->nextnode;
	    }
	}
      else
	{
	  trace(TRACE_MESSAGE,"bounce(): "
		"Message does not have a Return-Path nor a From headerfield, bounce failed");
	break;
      }
    }

  return 0;
}

