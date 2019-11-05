//==============================================================================
//
//  OvenMediaEngine
//
//  Created by Hyunjun Jang
//  Copyright (c) 2019 AirenSoft. All rights reserved.
//
//==============================================================================
#include "hls_stream_server.h"
#include "hls_private.h"

HttpConnection HlsStreamServer::ProcessRequestStream(const std::shared_ptr<HttpClient> &client,
													 const ov::String &app_name, const ov::String &stream_name,
													 const ov::String &file_name, const ov::String &file_ext)
{
	auto &response = client->GetResponse();

	if (file_name == HLS_PLAYLIST_FILE_NAME)
	{
		return OnPlayListRequest(client, app_name, stream_name, file_name, PlayListType::M3u8);
	}
	else if (file_ext == HLS_SEGMENT_EXT)
	{
		return OnSegmentRequest(client, app_name, stream_name, file_name, SegmentType::MpegTs);
	}

	response->SetStatusCode(HttpStatusCode::NotFound);
	response->Response();

	return HttpConnection::Closed;
}

HttpConnection HlsStreamServer::OnPlayListRequest(const std::shared_ptr<HttpClient> &client,
												  const ov::String &app_name, const ov::String &stream_name,
												  const ov::String &file_name,
												  PlayListType play_list_type)
{
	auto &response = client->GetResponse();

	ov::String play_list;

	auto item = std::find_if(_observers.begin(), _observers.end(),
							 [&app_name, &stream_name, &file_name, &play_list](auto &observer) -> bool {
								 return observer->OnPlayListRequest(app_name, stream_name, file_name, play_list);
							 });

	if (play_list.IsEmpty())
	{
		logtd("Could not find HLS playlist: %s/%s, %s", app_name.CStr(), stream_name.CStr(), file_name.CStr());
		response->SetStatusCode(HttpStatusCode::NotFound);
		response->Response();

		return HttpConnection::Closed;
	}

	// Set HTTP header
	response->SetHeader("Content-Type", "application/x-mpegURL");
	response->AppendString(play_list);
	response->Response();

	return HttpConnection::Closed;
}

HttpConnection HlsStreamServer::OnSegmentRequest(const std::shared_ptr<HttpClient> &client,
												 const ov::String &app_name, const ov::String &stream_name,
												 const ov::String &file_name,
												 SegmentType segment_type)
{
	auto &response = client->GetResponse();

	std::shared_ptr<SegmentData> segment = nullptr;

	auto item = std::find_if(_observers.begin(), _observers.end(),
							 [&app_name, &stream_name, &file_name, &segment](auto &observer) -> bool {
								 return observer->OnSegmentRequest(app_name, stream_name, file_name, segment);
							 });

	if (item == _observers.end())
	{
		logtd("Could not find HLS segment: %s/%s, %s", app_name.CStr(), stream_name.CStr(), file_name.CStr());
		response->SetStatusCode(HttpStatusCode::NotFound);
		response->Response();

		return HttpConnection::Closed;
	}

	// Set HTTP header
	response->SetHeader("Content-Type", "video/MP2T");
	response->AppendData(segment->data);
	response->Response();

	return HttpConnection::Closed;
}
