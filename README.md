# MAODV - Node Clustering AODV

**Implementasi NS2 dari Paper "An Optimized AODV Protocol Based on Clustering for WSNs"**

     Naufal Pranasetyo Fodensi
     5115100057
     Kelas Jaringan Nirkabel 2018
---
#### Penjelasan singkat modifikasi AODV
Paper berjudul **An Optimized AODV Protocol Based on Clustering in WSNs** ini mengusulkan suatu algoritma clustering yang bernama AODV based on Node Degree Clustering and Second Search (AODVNDC-SS) untuk mengontrol mekanisme flooding saat membangun suatu rute baru di protokol AODV. Di dalam algoritma ini node dikelompokkan sesuai dengan derajat nodenya untuk mengurangi propagasi pesan control. Node yang memiliki jumlah tetangga terbanyak akan menjadi **Cluster Head**. Pada saat yang sama, digunakan **gateway umum** atau gateway kooperatif untuk menghubungkan dua cluster heads (CHs) untuk mengurangi konsumsi energi cluster heads. Dengan menggunakan algoritma clustering ini akan menghemat energi dan memperpanjang network lifetime dalam jaringan sensor nirkabel.

#### Modifikasi yang akan dilakukan

1. Membuat global variable baru yang berfungsi untuk menyipan jumlah tetangga dari tiap node dalam *aodv_packet.h* pada hdr_aodv_reply berupa **rp_chid** dan menggunakan ++ nb_insert dan -- nb_delete.
2. Menambah field dan array **CH** dan **NodeNeighbor** untuk menyimpan ID node cluster head dan jumlah tetangga tiap node pada *recv request dan recv reply*
3. Menambah atribut koordinat dan **CH ID** neighbor node pada class AODV_Neighbor dalam file *aodv_rtable.h*
4. Melakukan request ke node tetangga dan simpan informasi dari node tersebut berupa berapa jumlah tetangganya, dan labeli node dengan jumlah tetangga terbanyak menjadi **Cluster Head** menggunakan logika "if else" pada recv reply pada *aodv.cc*
5. Cluster Head Melakukan re-broadcast **RE-RREQ** ke node lain untuk mendeklarasikan dirinya ialah Cluster Head pada recv request
6. Jika ada node penghubung antara dua Cluster Head, node tersebut menjadi gateway biasa. Jika tidak, maka menjadi **Cooperative Gateway** yang berguna untuk menjamin komunikasi antar node
7. Ketika node destinasi menerima pesan RE-RREQ. Node tujuan mengirim ulang pesan RREP (RE-RREP) ke node sumber dengan reverse route, ketika node sumber menerima pesan RE-RREP, rute lebih pendek akan dibuat.

#### Referensi
- https://ieeexplore.ieee.org/document/8343729
- https://arxiv.org/pdf/1007.4065.pdf
- http://intip.in/RA2018