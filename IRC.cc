
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

GtkListStore * list_rooms;
GtkListStore * room_user_names;
GtkWidget * room_entry;
GtkWidget * create_room;
GtkWidget * namelist;

char * usern;
char * passw;
GtkWidget *pass;



void update_list_rooms() {
    GtkTreeIter iter;
    int i;

    /* Add some messages to the window */
    for (i = 0; i < 10; i++) {
        gchar *msg = g_strdup_printf ("Room %d", i);
        gtk_list_store_append (GTK_LIST_STORE (list_rooms), &iter);
        gtk_list_store_set (GTK_LIST_STORE (list_rooms), 
	                    &iter,
                            0, msg,
	                    -1);
	g_free (msg);
    }
}

void update_room_user_names() {
    GtkTreeIter iter;
    int i;

    /* Add some messages to the window */
    for (i = 0; i < 10; i++) {
        gchar *msg = g_strdup_printf ("User %d", i);
        gtk_list_store_append (GTK_LIST_STORE (room_user_names), &iter);
        gtk_list_store_set (GTK_LIST_STORE (room_user_names), &iter, 0, msg,-1);
		g_free (msg);
    }
}


/* Create the list of "messages" */
static GtkWidget *create_list( const char * titleColumn, GtkListStore *model )
{
    GtkWidget *scrolled_window;
    GtkWidget *tree_view;
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
   gtk_text_buffer_insert (buffer, &iter, initialText,-1);
}
   
/* Create a scrolled text area that displays a "message" */
static GtkWidget *create_text( const char * initialText )
{
   GtkWidget *scrolled_window;
   GtkWidget *view;
   GtkTextBuffer *buffer;

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


char * host;
char * user;
char * password;
char * sport;
int port = 8888;

#define MAX_MESSAGES 100
#define MAX_MESSAGE_LEN 300
#define MAX_RESPONSE (20 * 1024)

int lastMessage = 0;

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

int sendCommand(char * host, int port, char * command, char * user, char * password, char * args, char * response) {
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

	//printf("response:%s\n", response);

	close(sock);
}

void printUsage()
{
	printf("Usage: talk-client host port user password\n");
	exit(1);
}

void add_user() {
	// Try first to add user in case it does not exist.
	char response[MAX_RESPONSE];
	sendCommand(host, port, "ADD-USER", usern, passw, "", response);
	
	if (!strcmp(response,"OK\r\n")) {
		printf("User %s added\n", user);
	}
}

void fncreate_room() {
	// Try first to add user in case it does not exist.
	char response[MAX_RESPONSE];
	sendCommand(host, port, "CREATE-ROOM", usern, passw, "", response);
	
	if (!strcmp(response,"OK\r\n")) {
		printf("Room %s added\n", user);
	}
}






static gboolean delete_event( GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   data )
{
    /* If you return FALSE in the "delete-event" signal handler,
     * GTK will emit the "destroy" signal. Returning TRUE means
     * you don't want the window to be destroyed.
     * This is useful for popping up 'are you sure you want to quit?'
     * type dialogs. */

    g_print ("delete event occurred\n");

    /* Change TRUE to FALSE and the main window will be destroyed with
     * a "delete-event". */

    return FALSE;
}



static void enter_callback( GtkWidget *widget,
                            GtkWidget *entry )
{
  const gchar *entry_text;
  entry_text = gtk_entry_get_text (GTK_ENTRY (entry));
  printf ("Entry contents: %s\n", entry_text);
  usern = (char *)entry_text;
}

char * sendrn;

static void room_enter_callback( GtkWidget *widget,
                            GtkWidget *entry )
{
  const gchar *entry_text;
  entry_text = gtk_entry_get_text (GTK_ENTRY (entry));
  printf ("Entry contents: %s\n", entry_text);
  sendrn = (char *)entry_text;

}


static void pass_callback( GtkWidget *widget,
                            GtkWidget *entry )
{
  const gchar *entry_text;
  entry_text = gtk_entry_get_text (GTK_ENTRY (entry));
  printf ("Entry contents: %s\n", entry_text);
  passw = (char *)entry_text;
}

static void send_details( GtkWidget *widget, GtkWidget *w1)
{
  const gchar *entry_text;
  entry_text = gtk_entry_get_text (GTK_ENTRY (w1));
  printf ("Entry contents: %s\n", entry_text);
  usern = (char *)entry_text;
  const gchar *entry_text2;
  entry_text2 = gtk_entry_get_text (GTK_ENTRY (pass));
  printf ("Entry contents: %s\n", entry_text2);
  passw = (char *)entry_text2;
  add_user();
  
}

static void send_create_room(GtkWidget *widget, GtkWidget *w1)
{
  const gchar *entry_text;
  entry_text = gtk_entry_get_text (GTK_ENTRY (w1));
  printf ("Entry contents: %s\n", entry_text);
  sendrn = (char *)entry_text;
  fncreate_room();
  
}


static void hello( GtkWidget *widget,
                   gpointer   data )
{
    //g_print ("Hello World\n");
	GtkWidget *window;
	GtkWidget *username;
	gint tmp_pos;
	
    //gtk_init (&argc, &argv);
   
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "Encrypted Login Window");
    g_signal_connect (window, "delete-event",
	              G_CALLBACK (delete_event), NULL);
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    gtk_widget_set_size_request (GTK_WIDGET (window), 400, 250);

    // Create a table to place the widgets. Use a 7x4 Grid (7 rows x 4 columns)
    GtkWidget *table = gtk_table_new (4, 3, TRUE);
    gtk_container_add (GTK_CONTAINER (window), table);
    gtk_table_set_row_spacings(GTK_TABLE (table), 5);
    gtk_table_set_col_spacings(GTK_TABLE (table), 5);
    gtk_widget_show (table);

	username = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (username), 50);
    g_signal_connect (username, "activate", G_CALLBACK (enter_callback), username);
    gtk_entry_set_text (GTK_ENTRY (username), "Username");
    tmp_pos = GTK_ENTRY (username)->text_length;
   // gtk_editable_insert_text (GTK_EDITABLE (entry), " world", -1, &tmp_pos);
   // gtk_editable_select_region (GTK_EDITABLE (entry),
	//		        0, GTK_ENTRY (entry)->text_length);
   // gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 0);
	gtk_table_attach_defaults(GTK_TABLE (table), username, 0, 4, 0, 1); 
    gtk_widget_show (username);

	pass = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (pass), 50);
    g_signal_connect (pass, "activate", G_CALLBACK (pass_callback), pass);
    gtk_entry_set_text (GTK_ENTRY (pass), "Password");
    tmp_pos = GTK_ENTRY (pass)->text_length;
	gtk_entry_set_visibility (GTK_ENTRY (pass),FALSE);
   // gtk_editable_insert_text (GTK_EDITABLE (entry), " world", -1, &tmp_pos);
   // gtk_editable_select_region (GTK_EDITABLE (entry),
	//		        0, GTK_ENTRY (entry)->text_length);
   // gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 0);
	gtk_table_attach_defaults(GTK_TABLE (table), pass, 0, 4, 1, 2); 
    gtk_widget_show (pass);
	
    // Add send button. Use columns 0 to 1 (exclusive) and rows 4 to 7 (exclusive)
    GtkWidget *send_button = gtk_button_new_with_label ("Create Account");
    gtk_table_attach_defaults(GTK_TABLE (table), send_button, 0, 2, 2, 3); 
	//g_signal_connect_swapped (send_button, "clicked",
	//		      G_CALLBACK (gtk_widget_destroy),
    //                          window);
    gtk_widget_show (send_button);

	g_signal_connect (send_button, "clicked", G_CALLBACK (send_details), username);

	 // Add ca button. Use columns 0 to 1 (exclusive) and rows 4 to 7 (exclusive)
    GtkWidget *cancel = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
	g_signal_connect_swapped (cancel, "clicked",
			      G_CALLBACK (gtk_widget_destroy),
			      window);
	/*= gtk_button_new_with_label ("Close"); */
    gtk_table_attach_defaults(GTK_TABLE (table), cancel, 2, 4, 2, 3); 
    gtk_widget_show (cancel);
   
	//gtk_widget_show (cancel);

	//g_signal_connect (cancel, "clicked", G_CALLBACK (delete_event), NULL);
	
    
    gtk_widget_show (table);
    gtk_widget_show (window);

   // gtk_main ();

   // return 0;
}


int main( int   argc,
          char *argv[] )
{
    GtkWidget *window;
    GtkWidget *list;
    GtkWidget *messages;
    GtkWidget *myMessage;

	host = "localhost";

	//up = (userpass *)malloc(100*sizeof(userpass));

	usern = (char *)g_malloc(100);
	passw = (char *)g_malloc(100);
	sendrn = (char *)g_malloc(100);

    gtk_init (&argc, &argv);
   
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "Paned Windows");
    g_signal_connect (window, "destroy",
	              G_CALLBACK (gtk_main_quit), NULL);
    gtk_container_set_border_width (GTK_CONTAINER (window), 10);
    gtk_widget_set_size_request (GTK_WIDGET (window), 800, 650);

    // Create a table to place the widgets. Use a 7x4 Grid (7 rows x 4 columns)
    GtkWidget *table = gtk_table_new (10, 9, TRUE);
    gtk_container_add (GTK_CONTAINER (window), table);
    gtk_table_set_row_spacings(GTK_TABLE (table), 7);
    gtk_table_set_col_spacings(GTK_TABLE (table), 7);
    gtk_widget_show (table);

    // Add list of rooms. Use columns 0 to 4 (exclusive) and rows 0 to 4 (exclusive)
    list_rooms = gtk_list_store_new (1, G_TYPE_STRING);
    update_list_rooms();
    list = create_list ("Rooms", list_rooms);
    gtk_table_attach_defaults (GTK_TABLE (table), list, 0, 2, 0, 4);
    gtk_widget_show (list);
   
	room_user_names = gtk_list_store_new (7, G_TYPE_STRING);
    update_room_user_names();
    namelist = create_list ("Users", room_user_names);
    gtk_table_attach_defaults (GTK_TABLE (table), namelist, 0, 2, 6, 10);
    gtk_widget_show (namelist);

	room_entry = gtk_entry_new ();
    gtk_entry_set_max_length (GTK_ENTRY (room_entry), 15);
    g_signal_connect (room_entry, "activate", G_CALLBACK (room_enter_callback), room_entry);
    gtk_entry_set_text (GTK_ENTRY (room_entry), "Room name");
    //tmp_pos = GTK_ENTRY (room_name)->text_length;
   // gtk_editable_insert_text (GTK_EDITABLE (entry), " world", -1, &tmp_pos);
   // gtk_editable_select_region (GTK_EDITABLE (entry),
	//		        0, GTK_ENTRY (entry)->text_length);
   // gtk_box_pack_start (GTK_BOX (vbox), entry, TRUE, TRUE, 0);
	gtk_table_attach_defaults(GTK_TABLE (table), room_entry, 0, 2, 4, 5); 
    gtk_widget_show (room_entry);

	// Add create_room button. Use columns 0 to 1 (exclusive) and rows 4 to 7 (exclusive)
    GtkWidget *create_room = gtk_button_new_with_label ("Create Room");
    gtk_table_attach_defaults(GTK_TABLE (table), create_room, 0, 2, 5, 6); 
    gtk_widget_show (create_room);

	g_signal_connect (create_room, "clicked", G_CALLBACK (send_create_room), room_entry);
	
    // Add messages text. Use columns 0 to 4 (exclusive) and rows 4 to 7 (exclusive) 
    messages = create_text ("Peter: Hi how are you\nMary: I am fine, thanks and you?\nPeter: Fine thanks.\n");
    gtk_table_attach_defaults (GTK_TABLE (table), messages, 2, 6, 0, 6);
    gtk_widget_show (messages);
    // Add messages text. Use columns 0 to 4 (exclusive) and rows 4 to 7 (exclusive) 

    myMessage = create_text ("I am fine, thanks and you?\n");
    gtk_table_attach_defaults (GTK_TABLE (table), myMessage, 2, 6, 7, 9);
    gtk_widget_show (myMessage);

    // Add send button. Use columns 0 to 1 (exclusive) and rows 4 to 7 (exclusive)
    GtkWidget *send_button = gtk_button_new_with_label ("Send");
    gtk_table_attach_defaults(GTK_TABLE (table), send_button, 2, 6, 9, 10); 
    gtk_widget_show (send_button);

	GtkWidget * ca= gtk_button_new_with_label ("Create Account");
    gtk_table_attach_defaults(GTK_TABLE (table), ca, 6, 9, 5, 6); 
    gtk_widget_show (ca);

	g_signal_connect (ca, "clicked", G_CALLBACK (hello), NULL);
	
	g_timeout_add(5000, (GSourceFunc) update_list_rooms, (gpointer) window);
	//ffff
    
    gtk_widget_show (table);
    gtk_widget_show (window);

    gtk_main ();

    return 0;
}

