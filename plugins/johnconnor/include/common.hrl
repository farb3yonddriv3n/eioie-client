-record(state, { logged :: atom(), fifowrite :: port() }).
-record(db_account, {email    :: binary(),
                     password :: binary(),
                     data     :: binary()}).
-record(proto_reply, {reply :: binary()}).
-record(proto_account_signup_reply, {error :: binary()}).
-record(proto_account_login_reply, {error :: binary()}).
-record(proto_account_logout_reply, {error :: binary()}).
-record(proto_broadcast, {pid :: binary(),
                          msg :: binary()}).
-record(proto_devices_online, {list :: list()}).
-record(proto_device_offline, {pid :: binary()}).

-record(proto_camera_request, {src :: binary(),
                               dst :: binary(),
                               snapshot :: integer()}).
-record(proto_camera_reinit, {src :: binary()}).
-record(proto_camera_response, {pid       :: binary(),
                                image     :: binary(),
                                objects   :: integer(),
                                timestamp :: integer(),
                                error     :: integer(),
                                snapshots :: integer(),
                                temp      :: list()}).
-record(proto_message, {data :: binary()}).
-record(proto_files_get, {src        :: binary(),
                          request_id :: integer()}).
-record(proto_job_add, {name       :: binary(),
                        request_id :: integer()}).
-record(proto_job_finalize, {name       :: binary(),
                             request_id :: integer()}).
-record(proto_peers_get, {request_id :: integer()}).
-record(proto_tunnel_open, {pubkeyhash :: binary(),
                            port       :: binary(),
                            request_id :: integer()}).
-record(proto_tunnel_dump, {request_id :: integer()}).
-record(proto_endpoint_dump, {request_id :: integer()}).
