{
  'targets': [
    {
      'target_name': 'bgpdump2',
      'sources': [
        'src/addon.cc',
        'src/bgpdump2.cc',
        'deps/bgpdump2/src/bgpdump_data.c',
        'deps/bgpdump2/src/bgpdump_file.c',
        'deps/bgpdump2/src/bgpdump_option.c',
        'deps/bgpdump2/src/bgpdump_parse.c',
        'deps/bgpdump2/src/bgpdump_peer.c',
        'deps/bgpdump2/src/bgpdump_peerstat.c',
        'deps/bgpdump2/src/bgpdump_query.c',
        'deps/bgpdump2/src/bgpdump_route.c',
        'deps/bgpdump2/src/ptree.c',
        'deps/bgpdump2/src/queue.c'
      ],
      'include_dirs': [
        '<!(node -e "require(\'nan\')")',
        'deps/bgpdump2/src'
      ],
      'libraries': [
        '-lbz2'
      ]
    }
  ]
}
