#include <stdio.h>
#include <time.h>
#include <curses.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <gtk/gtk.h>

#define MAX_MESSAGES 100
#define MAX_MESSAGE_LEN 300
#define MAX_RESPONSE (20 * 1024)

char * globalcopy = (char *)g_malloc(sizeof(char)*1000);

int genius = 0;

char names[1024] = "";
char rnames[1024] = "";
char msgz[MAX_RESPONSE ] = "";

int lastMessage = 0;

char * user1;
char * pass1;

char * host = strdup("localhost");
char * user = strdup("super");
char * password = strdup("clarkk");
char * sport;
int port = 2400;

char * roomname = (char *)g_malloc(sizeof(char)*1000);
char * prevname = strdup("q");

GtkWidget *tree_view;
GtkTreeSelection *selection;

GtkWidget *messages;
GtkWidget *view;
GtkTextBuffer *buffer;

GtkListStore * list_rooms;
GtkListStore * list_names;




/*
			           DO NOT TOUCH 
	___________________________________________________________________________________
	___________________________________________________________________________________
	___________________________________________________________________________________

 */

/* Create the list of "messages" */
static GtkWidget *create_list( const char * titleColumn, GtkListStore *model )
{
    GtkWidget *scrolled_window;
    //GtkListStore *model;
    GtkCellRenderer *cell;
    GtkTreeViewColumn *column;

    int i;
   
    /* Create a new scrolled window, with scrollbars only if needed */
    scrolled_window = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
				    GTK_POLICY_AUTOMATIC, 
				    GTK_POLICY_AUTOMATIC);
   
    //model = gtk_list_store_new (1, G_TYPE_STRING);
    tree_view = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (scrolled_window), tree_view);
    gtk_tree_view_set_model (GTK_TREE_VIEW (tree_view), GTK_TREE_MODEL (model));
    gtk_widget_show (tree_view);
   
    cell = gtk_cell_renderer_text_new ();

    column = gtk_tree_view_column_new_with_attributes (titleColumn,
                                                       cell,
                                                       "text", 0,
                                                       NULL);
  
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view),
	  		         GTK_TREE_VIEW_COLUMN (column));

    return scrolled_window;
}
   
/* Add some text to our text widget - this is a callback that is invoked
when our window is realized. We could also force our window to be
realized with gtk_widget_realize, but it would have to be part of
a hierarchy first */

static void insert_text( GtkTextBuffer *buffer, const char * initialText )
{
   GtkTextIter iter;
 
   gtk_text_buffer_get_iter_at_offset (buffer, &iter, 0);
   gtk_text_buffer_set_text (buffer, initialText,strlen(initialText));

	gtk_text_view_set_overwrite (GTK_TEXT_VIEW(view), TRUE);
}
   
/* Create a scrolled text area that displays a "message" */
static GtkWidget *create_text( const char * initialText )
{
   GtkWidget *scrolled_window;

   view = gtk_text_view_new ();
   buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

   scrolled_window = gtk_scrolled_window_new (NULL, NULL);
   gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
		   	           GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);

   gtk_container_add (GTK_CONTAINER (scrolled_window), view);
   insert_text (buffer, initialText);

   gtk_widget_show_all (scrolled_window);

   return scrolled_window;
}

int open_client_socket(char * host, int port) {
	// Initialize socket address structure
	struct  sockaddr_in socketAddress;
	
	// Clear sockaddr structure
	memset((char *)&socketAddress,0,sizeof(socketAddress));
	
	// Set family to Internet 
	socketAddress.sin_family = AF_INET;
	
	// Set port
	socketAddress.sin_port = htons((u_short)port);
	
	// Get host table entry for this host
	struct  hostent  *ptrh = gethostbyname(host);
	if ( ptrh == NULL ) {
		perror("gethostbyname");
		exit(1);
	}
	
	// Copy the host ip address to socket address structure
	memcpy(&socketAddress.sin_addr, ptrh->h_addr, ptrh->h_length);
	
	// Get TCP transport protocol entry
	struct  protoent *ptrp = getprotobyname("tcp");
	if ( ptrp == NULL ) {
		perror("getprotobyname");
		exit(1);
	}
	
	// Create a tcp socket
	int sock = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sock < 0) {
		perror("socket");
		exit(1);
	}
	
	// Connect the socket to the specified server
	if (connect(sock, (struct sockaddr *)&socketAddress,
		    sizeof(socketAddress)) < 0) {
		perror("connect");
		exit(1);
	}
	
	return sock;
}

int sendCommand(char * host, int port, char * command, char * user,
		char * password, char * args, char * response) {
	int sock = open_client_socket( host, port);

	// Send command
	write(sock, command, strlen(command));
	write(sock, " ", 1);
	write(sock, user, strlen(user));
	write(sock, " ", 1);
	write(sock, password, strlen(password));
	write(sock, " ", 1);
	write(sock, args, strlen(args));
	write(sock, "\r\n",2);

	// Keep reading until connection is closed or MAX_REPONSE
	int n = 0;
	int len = 0;
	while ((n=read(sock, response+len, MAX_RESPONSE - len))>0) {
		len += n;
	}
	response[len] = '\0';
	//printf("User: %s | Response sendCommand %s :%s\n",user,command, response);

	close(sock);
}

void printUsage()
{
	printf("Usage: talk-client host port user password\n");
	exit(1);
}

/*
					ALL USEFUL FUNCTIONS START HERE
 ___________________________________________________________________________________________________
 ___________________________________________________________________________________________________
 ___________________________________________________________________________________________________ 

*/

void update_list_rooms() {
    GtkTreeIter iter;
	gtk_list_store_clear(GTK_LIST_STORE (list_rooms));
	sendCommand(host, port, "LIST-ROOMS", user, password, "", rnames);
	char * token = strtok(rnames,"\r\n");
    while(token != NULL) {
		gchar *msg = g_strdup((gchar *)token);
        gtk_list_store_append (GTK_LIST_STORE (list_rooms), &iter);
        gtk_list_store_set (GTK_LIST_STORE (list_rooms), &iter, 0, msg, -1);
		g_free (msg);
		token = strtok(NULL, "\r\n");
    }
}

static void loginwindow(GtkWidget *widget, GtkWindow *data) {
	GtkWidget *window, *table;
	GtkWidget *entryu, *entryp;
	GtkWidget *labelu, *labelp;
	gint response;
	
	window = gtk_dialog_new_with_buttons ("Encrypted Login Window", NULL, GTK_DIALOG_MODAL, GTK_STOCK_OK, 		GTK_RESPONSE_OK, GTK_STOCK_CANCEL,GTK_RESPONSE_CANCEL,NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(window), GTK_RESPONSE_OK);

    labelu = gtk_label_new("Username: ");
    labelp = gtk_label_new("Password: ");
    entryu = gtk_entry_new();
    entryp = gtk_entry_new();

    table = gtk_table_new (2, 2, FALSE);
    gtk_table_attach_defaults(GTK_TABLE (table), labelu, 0, 1, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE (table), labelp, 0, 1, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE (table), entryu, 1, 2, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE (table), entryp, 1, 2, 1, 2);

    gtk_container_set_border_width(GTK_CONTAINER(table), 5);
    gtk_box_pack_start_defaults(GTK_BOX  (GTK_DIALOG (window)->vbox), table);

    gtk_widget_show_all(window);
    response = gtk_dialog_run (GTK_DIALOG(window));
    if(response == GTK_RESPONSE_OK) {
        //g_print("The username is: %s\n", gtk_entry_get_text (GTK_ENTRY (entryu)));
        //g_print("The password is: %s\n", gtk_entry_get_text (GTK_ENTRY (entryp)));
        user1 = (char *)gtk_entry_get_text (GTK_ENTRY (entryu));
        pass1 = (char *)gtk_entry_get_text (GTK_ENTRY (entryp));
        char response[ MAX_RESPONSE ];
		user = strdup(user1);
		password = strdup(pass1);
		sendCommand(host, port, "ADD-USER", user, password, "", response);
		//printf("User in Account creation: %s \n",user);
		//if (!strcmp(response,"OK\r\n")) printf("User %s added\n", user);
    }
	update_list_rooms();
	gtk_widget_destroy(window);
}

void update_list_names() {
	GtkTreeIter iter;
	gtk_list_store_clear(GTK_LIST_STORE (list_names));
	if(roomname != NULL && user != NULL && password != NULL) {
		sendCommand(host, port, "GET-USERS-IN-ROOM", user, password, roomname, names);
		char * hi = strdup(names);
		char * token = strtok(hi,"\r\n");
   		while(token != NULL) {
			printf("Token: %s\n",token);
			gchar *msg = g_strdup((gchar *)token);
      	 	gtk_list_store_append (GTK_LIST_STORE (list_names), &iter);
      	 	gtk_list_store_set (GTK_LIST_STORE (list_names), &iter, 0, msg, -1);
			g_free (msg);
			token = strtok(NULL, "\r\n");
  	 	}
	}
}

void getmsgs() {
	if(genius == 1) {
		char blah[20];
		sprintf(blah,"%d",lastMessage);

		char * firststring = strcat(blah," ");
		if(roomname != NULL && user != NULL && password != NULL && buffer != NULL) {
			char * secondstring = strcat(firststring,strdup(roomname));
			sendCommand(host, port, "GET-MESSAGES", user, password, secondstring, msgz);
			insert_text(buffer,msgz);
			//lastMessage++;
		}
	}	
}



void on_changed(GtkWidget *widget, gpointer label) 
{
	genius = 1;
  GtkTreeIter iter;
  GtkTreeModel *model;
  char er[200];
  if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
	gtk_tree_model_get(model, &iter, 0, &roomname,  -1);
    gtk_label_set_text(GTK_LABEL(label), roomname);
  }
  if(roomname == NULL) return;
  if(!strcmp(roomname,globalcopy))return;	
  if(prevname !=NULL){
	 char response[MAX_RESPONSE];
	 if(user != NULL && roomname != NULL) sprintf(er,"%s is leaving the room. GoodBye!",prevname);
		printf("Hi pls\n");
		sendCommand(host, port, "SEND-MESSAGE", user, password, er, response);
				printf("Send message: %s\n",response);
			printf("User: %s Room: %s\n",user,prevname);
		sendCommand(host, port, "LEAVE-ROOM", user, password, prevname, response);
		printf("Leave room: %s\n",response);
		printf("User: %s Room: %s\n",user,prevname);
  }
  globalcopy = strdup(roomname);
  char response[MAX_RESPONSE];
  if(user != NULL && roomname != NULL) sprintf(er,"%s has joined the room. Go ahead and chat!",roomname);
	sendCommand(host, port, "ENTER-ROOM", user, password, roomname, response);
	sendCommand(host, port, "SEND-MESSAGE", user, password, er, response);	
	getmsgs();
	update_list_names();
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	getmsgs();
	prevname = strdup(roomname);
}

static void sendMessg (GtkWidget *widget, GtkWidget *entry) {
	const char *entry_text;
    entry_text = gtk_entry_get_text (GTK_ENTRY (entry));
    printf ("Entry contents: %s\n", entry_text);
   // user = (char *)entry_text;
	char * entryy = strdup(entry_text);

	char response[MAX_RESPONSE];
	char * u3 = strcat(strdup(roomname)," ");
	char * u4 = strcat(u3,entryy);
	//printf("u3: %s\n",u3);
	getmsgs();
	sendCommand(host, port, "SEND-MESSAGE", user, password, strdup(u4), response);
	getmsgs();
	//printf("User in sendmsg: %s \n",user);
	//printf("Response sendmsg: %s\n",response);
}

static void create_room (GtkWidget *widget, GtkWidget *entry ) {
	const char *entry_text;
    entry_text = gtk_entry_get_text (GTK_ENTRY (entry));
    //printf ("Entry contents: %s\n", entry_text);

	char response[MAX_RESPONSE];
	char * u3 = strdup(entry_text);
	sendCommand(host, port, "CREATE-ROOM", user, password, strdup(u3), response);
	//printf("User in Create room: %s ",user);
	//printf("Response Create Room: %s",response);
	update_list_rooms();
}

static void clearing (GtkWidget *widget, GtkWidget *entry) {
	 gtk_entry_set_text (GTK_ENTRY (entry), "");	
}

static gboolean
time_handler(GtkWidget *widget)
{
 if (widget->window == NULL) return FALSE;
if(genius == 1) {
 

  //gtk_widget_queue_draw(widget);

 // fprintf(stderr, "Hi\n");
  update_list_rooms();
  getmsgs();
  update_list_names();
	//}
  return TRUE;
}

return TRUE;
}

static void create_room1 (GtkWidget *widget, GtkWidget *entry ) {}

int main(int argc, char *argv[] )
{
	if(argc >=2) {
		host = argv[1];
		port = atoi(argv[2]);
	}	

    GtkWidget *window;
    GtkWidget *list;
	GtkWidget *list2;
    GtkWidget *myMessage;
	GtkWidget *sendlabel;
	GtkWidget *random;

	GtkWidget *R_entry;
	GtkWidget *croom;
	GtkWidget *cacc;
	GtkWidget *text;

	prevname = NULL;

    gtk_init (&argc, &argv);
   
	// Window
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "IRCClient");
    g_signal_connect (window, "destroy",
	              G_CALLBACK (gtk_main_quit), NULL);
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    gtk_widget_set_size_request (GTK_WIDGET (window), 800, 650);

	

    // Table 10x10
    GtkWidget *table = gtk_table_new (10, 10, TRUE);
    gtk_container_add (GTK_CONTAINER (window), table);
    gtk_table_set_row_spacings(GTK_TABLE (table), 7);
    gtk_table_set_col_spacings(GTK_TABLE (table), 7);
    gtk_widget_show (table);



    // list_rooms LIST
	text = gtk_label_new("");
    list_rooms = gtk_list_store_new (1, G_TYPE_STRING);
    //update_list_rooms();
    list = create_list ("Rooms", list_rooms);
    gtk_table_attach_defaults (GTK_TABLE (table), list, 0, 3, 0, 4);
    gtk_widget_show (list);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	g_signal_connect(selection, "changed", G_CALLBACK(on_changed), text);


	//list_names LIST
	list_names = gtk_list_store_new (1, G_TYPE_STRING);
    //update_list_names();
    list2 = create_list ("Users in Room", list_names);
    gtk_table_attach_defaults (GTK_TABLE (table), list2, 0, 3, 6, 10);
    gtk_widget_show (list2);
	


   
	// Room ENTRY
	R_entry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (R_entry), 15);
    g_signal_connect (R_entry, "activate", G_CALLBACK (create_room1), R_entry);
    gtk_entry_set_text (GTK_ENTRY (R_entry), "Room name");	
	gtk_table_attach_defaults(GTK_TABLE (table), R_entry, 0, 3, 4, 5); 
    gtk_widget_show (R_entry);




	// Create Room BUTTON
    croom = gtk_button_new_with_label ("Create Room");
    gtk_table_attach_defaults(GTK_TABLE (table), croom, 0, 3, 5, 6); 
    gtk_widget_show (croom);
	g_signal_connect (croom, "clicked", G_CALLBACK (create_room), R_entry);



	// Create account BUTTON
	cacc = gtk_button_new_with_label ("Create Account / Sign In");
    gtk_table_attach_defaults(GTK_TABLE (table), cacc, 7, 10, 5, 6); 
    gtk_widget_show (cacc);
	g_signal_connect (cacc, "clicked", G_CALLBACK (loginwindow), NULL);


	
    // Add messages text. Use columns 0 to 4 (exclusive) and rows 4 to 7 (exclusive) 
   // messages = create_text ("Peter: Hi how are you\nMary: I am fine, thanks and you?\nPeter: Fine thanks.\n");
	messages = create_text("Select a room and Messages will be displayed here.\n");    
	gtk_table_attach_defaults (GTK_TABLE (table), messages, 3, 7, 0, 7);
    gtk_widget_show (messages);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW(view), GTK_WRAP_WORD);
	
	// LABEL for send
	sendlabel = gtk_label_new("Type your message below: (Enter or SEND)");
	gtk_table_attach_defaults(GTK_TABLE (table), sendlabel, 3, 7, 7, 8);
	gtk_widget_show(sendlabel);

	//Label for Right most corner:
	//random = gtk_label_new("");
	//gtk_table_attach_defaults(GTK_TABLE (table), random, 7, 10, 2, 3);
	//gtk_widget_show(random);

	GtkWidget *random2 = gtk_label_new("Access restricted to members only.");
	gtk_table_attach_defaults(GTK_TABLE (table), random2, 7, 10, 3, 4);
	gtk_widget_show(random2);

	


    // Add messages text. Use columns 0 to 4 (exclusive) and rows 4 to 7 (exclusive) 
    myMessage = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (myMessage), MAX_MESSAGE_LEN);
    g_signal_connect (myMessage, "activate", G_CALLBACK (sendMessg), myMessage);
	g_signal_connect (myMessage, "activate", G_CALLBACK (clearing), myMessage);
    gtk_entry_set_text (GTK_ENTRY (myMessage), "");	
	gtk_table_attach_defaults(GTK_TABLE (table), myMessage, 3, 7, 8, 9); 
    gtk_widget_show (myMessage);


   // gtk_table_attach_defaults (GTK_TABLE (table), myMessage, 3, 7, 7, 9);
   // gtk_widget_show (myMessage);

    // SEND BUTTON
    GtkWidget *send_button = gtk_button_new_with_label ("Send");
    gtk_table_attach_defaults(GTK_TABLE (table), send_button, 3, 7, 9, 10); 
	g_signal_connect (send_button, "clicked", G_CALLBACK (sendMessg), myMessage);
	g_signal_connect (send_button, "clicked", G_CALLBACK (clearing), myMessage);
	gtk_widget_show (send_button);    
	
    gtk_widget_show (table);
    gtk_widget_show (window);

	g_timeout_add(5000, (GSourceFunc) time_handler, (gpointer) window);

    gtk_main ();

    return 0;
}

