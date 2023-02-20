load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_java//java:defs.bzl", "java_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

cc_binary(
    name = "client",
    srcs = ["//src:client.cc"],
    deps = [chat_cc_proto],
)

cc_binary(
    name = "server",
    srcs = ["//src:server.cc"],
)

cc_proto_library(
    name = "chat_cc_proto",
    deps = [":chat_proto"],
)

proto_library(
    name= "chat_proto",
    srcs = ["//protos:chat.proto"],
)