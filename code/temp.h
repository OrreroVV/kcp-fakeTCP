int send_handshake_syn_packet(struct fake_tcp_client* client)
{
    char buffer[BUFFER_SIZE] = { 0 };

    build_ip_header(buffer, IPV4_TCP_HEADER_LEN, client->ip_id, &client->source,
    &client->target);
    client->ip_id++;

    build_tcp_header(buffer + IPV4_HEADER_LEN, TCP_HEADER_LEN, &client->source,
    &client->target, 0, 0, 1, 0, 0);

    int n = tun_write(client->tun_fd, buffer, IPV4_TCP_HEADER_LEN);
    if (n < 0)
    {
        printf("tuntap_write() failed: %s\n", strerror(errno));
        return -1;
    }

    printf("Sent Handshake SYN packet, seq=%u, ack=%u\n", client->tcp_seq,
    client->server_tcp_seq);

    return 0;
}

int parse_handshake_syn_ack_packet(struct fake_tcp_client* client, char* data, int len)
{
    struct iphdr* iph = (struct iphdr*)data;
    if (iph->protocol != IPPROTO_TCP)
    {
        printf("Ingore not tcp packet\n");
        return -1;
    }

    struct tcphdr* tcph = (struct tcphdr*)(data + IPV4_HEADER_LEN);
    if (tcph->syn == 1 && tcph->ack == 1)
    {
        client->tcp_seq = ntohl(tcph->ack_seq);
        client->server_tcp_seq = ntohl(tcph->seq);

        printf("Received Handshake SYN-ACK packet, seq=%u, ack=%u\n",
        client->server_tcp_seq, client->tcp_seq);

        return 0;
    }
    else
    {
        printf("Received not SYN-ACK packet\n");
        return -1;
    }
}

int wait_handshake_syn_ack_packet(struct fake_tcp_client* client)
{
    int fd = client->tun_fd;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    int ret = select(fd + 1, &readfds, NULL, NULL, &tv);
    if (ret <= 0)
    {
        printf("Wait SYN-ACK packet timeout\n");
        return -1;
    }
    else
    {
        char buffer[BUFFER_SIZE];
        int n = tun_read(client->tun_fd, buffer, BUFFER_SIZE);
        if (n < 0)
        {
            printf("tuntap_read() failed: %s\n", strerror(errno));
            return -1;
        }

        return parse_handshake_syn_ack_packet(client, buffer, n);
    }
}

int send_handshake_ack_packet(struct fake_tcp_client* client)
{
    char buffer[BUFFER_SIZE] = { 0 };

    build_ip_header(buffer, IPV4_TCP_HEADER_LEN, client->ip_id, &client->source,
    &client->target);
    client->ip_id++;

    build_tcp_header(buffer + IPV4_HEADER_LEN, IPV4_HEADER_LEN, &client->source,
    &client->target, client->tcp_seq, client->server_tcp_seq + 1, 0, 1, 0);

    int n = tun_write(client->tun_fd, buffer, IPV4_TCP_HEADER_LEN);
    if (n < 0)
    {
        printf("tuntap_write() failed: %s\n", strerror(errno));
        return -1;
    }

    printf("Sent Handshake ACK packet, seq=%u, ack=%u\n", client->tcp_seq,
    client->server_tcp_seq + 1);
}

int handshake_with_fake_tcp_server(struct fake_tcp_client* client)
{
    int ret = send_handshake_syn_packet(client);
    if (ret < 0)
    {
        return -1;
    }

    ret = wait_handshake_syn_ack_packet(client);
    if (ret < 0)
    {
        return -1;
    }

    ret = send_handshake_ack_packet(client);
    if (ret < 0)
    {
        return -1;
    }

    printf("Handshake with server completed\n");

    return 0;
}

int parse_data_ack_packet(struct fake_tcp_client* client, char* data, int len)
{
    struct iphdr* iph = (struct iphdr*)data;
    if (iph->protocol != IPPROTO_TCP)
    {
        printf("Ingore not tcp packet\n");
        return -1;
    }

    struct tcphdr* tcph = (struct tcphdr*)(data + IPV4_HEADER_LEN);
    if (tcph->ack == 1)
    {
        client->tcp_seq = ntohl(tcph->ack_seq);

        printf("Received DATA-ACK packet, seq=%u, ack=%u\n",
        client->server_tcp_seq, client->tcp_seq);

        return 0;
    }
    else
    {
        exit(0);
        printf("Received not DATA-ACK packet\n");
        return -1;
    }
}

int wait_data_ack_packet(struct fake_tcp_client* client)
{
    int fd = client->tun_fd;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    int ret = select(fd + 1, &readfds, NULL, NULL, &tv);
    if (ret <= 0)
    {
        printf("Wait DATA-ACK timeout\n");
        return -1;
    }
    else
    {
        char buffer[BUFFER_SIZE];
        int n = tun_read(client->tun_fd, buffer, BUFFER_SIZE);
        if (n < 0)
        {
            printf("tuntap_read() failed: %s\n", strerror(errno));
            return -1;
        }

        return parse_data_ack_packet(client, buffer, n);
    }

    return 0;
}

int init_fake_tcp_client(struct fake_tcp_client* client)
{
    client->tun_fd = setup_tun_device("tun101", "10.0.0.1", "255.255.255.0");
    if (client->tun_fd < 0)
    {
        return -1;
    }

    client->ip_id = 0;
    client->tcp_seq = 0;
    client->server_tcp_seq = 0;

    return 0;
}

void deinit_fake_tcp_client(struct fake_tcp_client* client)
{
    close_tun_device(client->tun_fd);
}

int connect_to_fake_tcp_server(struct fake_tcp_client* client,
const char* src_ip,
uint16_t src_port,
const char* dst_ip,
uint16_t dst_port)
{
    client->source.sin_family = AF_INET;
    client->source.sin_port = htons(src_port);
    client->source.sin_addr.s_addr = inet_addr(src_ip);

    client->target.sin_family = AF_INET;
    client->target.sin_port = htons(dst_port);
    client->target.sin_addr.s_addr = inet_addr(dst_ip);

    return handshake_with_fake_tcp_server(client);
}

int send_data_to_fake_tcp_server(struct fake_tcp_client* client, char* data, int len)
{
    char buffer[BUFFER_SIZE] = { 0 };
    memcpy(buffer + IPV4_TCP_HEADER_LEN, data, len);

    int total_len = IPV4_TCP_HEADER_LEN + len;
    build_ip_header(buffer, total_len, client->ip_id, &client->source, &client->target);
    client->ip_id++;

    build_tcp_header(buffer + IPV4_HEADER_LEN, TCP_HEADER_LEN + len, &client->source,
    &client->target, client->tcp_seq, client->server_tcp_seq + 1, 0, 1, 1);

    while (1)
    {
        if (tun_write(client->tun_fd, buffer, total_len) < 0)
        {
            printf("tuntap_write() failed: %s\n", strerror(errno));
            return -1;
        }

        if (wait_data_ack_packet(client) == 0)
        {
            return 0;
        }
    }

    return -1;
}
