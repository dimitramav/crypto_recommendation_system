#include <iostream>
#include <sstream>
#include <iterator>
#include <string>
#include <random>
#include <cmath>
#include <numeric>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>	
#include <cmath> 
#include <numeric>
#include <limits>
#include <map>
#include <set>
#include <list>
#include <stdlib.h>
#include <random>
#include <ctime>
#include <utility>
#include "datastructs.h"

int is_nearest(double distance,DataVector * querypoint,int new_cluster) //calculates the cluster and the neighbour cluster of a datavector
{
	int current_cluster = querypoint->cluster_number_accessor().first;
	double first_minimum_distance = querypoint->cluster_number_accessor().second;
	int neighbour_cluster = querypoint->neighbour_cluster_accessor().first;
	double second_minimum_distance = querypoint->neighbour_cluster_accessor().second;
	if(distance<first_minimum_distance)
	{
		if(neighbour_cluster == new_cluster) //swap neighbour cluster with current cluster
			querypoint->change_neighbour_cluster(current_cluster,first_minimum_distance); 
		querypoint->change_cluster_number(new_cluster,distance);
		return 1;
	}
	else if(distance>first_minimum_distance && distance < second_minimum_distance)
	{
		if(current_cluster != new_cluster)  //avoid neighbour cluster to be equal to current cluster
			querypoint->change_neighbour_cluster(new_cluster,distance); 
	}
	return 0; 
}

map <DataVector *,double> trueNN(vector <DataVector *> dataset, DataVector * querypoint,string metric)
{
	DataVector * neighbour;
	int neighbour_cluster;
	int points_checked=0;
	map <DataVector * , double> nearest_neighbour;
	double distance;
	for (unsigned int i=0;i<dataset.size();i++)
	{
		distance=vectors_distance(metric,querypoint->point_accessor(),dataset[i]->point_accessor());
		if(is_nearest(distance, querypoint,i))
		{
			neighbour=dataset[i];
		}
		points_checked++;
			//cout << dataset[i]->name_accessor()<< "      " << distance << endl;
	}
	//cout << neighbour->name_accessor() <<" has nearest neighbour " << minimum_distance << endl;
	nearest_neighbour.insert ( pair<DataVector *,double>(neighbour,querypoint->cluster_number_accessor().second) );
	return nearest_neighbour;

}


void random_initialization(vector <DataVector *> & dataset_vector,vector <Cluster *> & cluster_vector, int k)
{
	set<int> random_k;
	set<int>::iterator it;

	srand(time(NULL));
	while (random_k.size() < k )
	{
		random_k.insert(rand() % dataset_vector.size());
	}
	for(it = random_k.begin(); it != random_k.end(); it++)
	{
        dataset_vector[*it]->set_centroid();   //set centroid DELETE IT 
        Cluster * cluster = new Cluster(dataset_vector[*it]);  //initialize new cluster with the random centroid
        cluster_vector.push_back(cluster);
    }
}

void set_centroid(vector <DataVector *> & dataset_vector,vector <Cluster*> & cluster_vector, int new_centroid)
{
	dataset_vector[new_centroid]->set_centroid();
    Cluster * cluster = new Cluster(dataset_vector[new_centroid]);
    cluster_vector.push_back(cluster);
}
void plus_initialization(vector <DataVector *> & dataset_vector, vector <Cluster *> & cluster_vector, int k,string metric)
{
	random_device rd;  //Will be used to obtain a seed for the random number engine
    mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    uniform_int_distribution<> dis(0, dataset_vector.size());
    vector <double> minimum_distance;
    double distance; 
    double max_sum;
    default_random_engine generator;
    //initialize vector with minimum distances
    for ( int point = 0;point<dataset_vector.size();point++)
    {
    	minimum_distance.push_back(numeric_limits<double>::max());
    }
    //initialize first centroid
    int starting_centroid = dis(gen);
    set_centroid(dataset_vector,cluster_vector,starting_centroid);
    for (int i=0; i<k-1 ;i++)  
    {
    	for ( int point = 0;point<dataset_vector.size();point++)
    	{
    		if(!dataset_vector[point]->centroid_accessor())  //if it is not centroid
    		{
    			for (int x=0;x<cluster_vector.size();x++)  //find minimum distance from current centroids
    			{
    				distance=vectors_distance(metric,dataset_vector[point]->point_accessor(),cluster_vector[x]->centroid_accessor()->point_accessor());
    				if(distance<minimum_distance[point])
    				{
    					minimum_distance[point] = distance; 
    					max_sum+= minimum_distance[point] * minimum_distance[point];
    				}
    			}
    		}
    		else 
    		{
    			minimum_distance[point] = 0.0;
    		}
    	}
    	uniform_real_distribution<> new_dis(0.0, max_sum);
    	double random_x = new_dis(generator);
    	max_sum=0;
    	for ( int point = 0;point<dataset_vector.size();point++)
    	{
    		max_sum+=minimum_distance[point];
    		if(max_sum > random_x)
    		{
    			int new_centroid = point; 
    			set_centroid(dataset_vector,cluster_vector,new_centroid);
    			break;
    		}
    	} 	
    }
}

void lloyds_assignment(vector <DataVector *> & dataset_vector,vector <Cluster *> & cluster_vector,string metric)
{
	map <DataVector *,double> true_neighbour;
	int old_cluster;
	vector <DataVector *> centroid_vector;
	vector <double> silhouette_vector; 
	for(unsigned int i=0;i<cluster_vector.size();i++)  //initialize vector with centroids for compatibility reasons
	{
		centroid_vector.push_back(cluster_vector[i]->centroid_accessor());
	}
	int counter =0;
	double new_objective_distance = 0.0;
	double previous_objective_distance = 1.0;
	do
	{	
		if (counter !=0)
		{
			previous_objective_distance = new_objective_distance;
		}
		for (unsigned int i = 0;i<cluster_vector.size();i++)
		{
			cluster_vector[i]->set_update(0);
		}
		new_objective_distance  = 0.0;
		old_cluster = -1; 
		for (unsigned int i=0;i<dataset_vector.size();i++)  //for each datapoint , find nearest centroid
		{
			if(dataset_vector[i]->cluster_number_accessor().first!=-1)  
			{
				old_cluster = dataset_vector[i]->cluster_number_accessor().first;
				//cout << "old_cluster " << old_cluster << endl;
				cluster_vector[old_cluster]->remove_from_cluster(dataset_vector[i]);
			}
			true_neighbour=trueNN(centroid_vector,dataset_vector[i],metric);
			//cout<< "true : "<<true_neighbour.begin()->first->name_accessor() << " :" << true_neighbour.begin()->second << endl;
					//cout << true_neighbour.begin()->first->name_accessor()<< "vs "<< cluster_vector[cluster]->centroid_accessor()->name_accessor() << endl	;
			int new_cluster = dataset_vector[i]->cluster_number_accessor().first;
			if (old_cluster != new_cluster)  //check if datapoint has changed cluster and affects a different cluster
			{
				if(old_cluster!=-1)
					cluster_vector[old_cluster]->set_update(1);
				cluster_vector[new_cluster]->set_update(1);
			}

			cluster_vector[new_cluster]->add_to_cluster(dataset_vector[i]); //add point to cluster	
			new_objective_distance += true_neighbour.begin()->second * true_neighbour.begin()->second;
			//getchar();

		}
		//getchar();
		/*for(unsigned int i=0;i<cluster_vector.size();i++)  //initialize vector with centroids for compatibility reasons
		{
			cluster_vector[i]->print_cluster();
			getchar();
		}*/
		cout << "objective_distance " << new_objective_distance << "/"<< previous_objective_distance<< endl;	
		//lloyds_update(cluster_vector);
		pam_update(cluster_vector,metric);
		for(unsigned int i=0;i<cluster_vector.size();i++)  //initialize vector with centroids for compatibility reasons
		{
			centroid_vector[i] = cluster_vector[i]->centroid_accessor();
		}
		counter++;
	}while(new_objective_distance!=	previous_objective_distance);
}


void lloyds_update(vector <Cluster *> & cluster_vector)
{
	vector <double > centroid_vector;
	DataVector * centroid_point;
	for(unsigned int i=0;i<cluster_vector.size();i++)  //vector with centroids for compatibility reasons
	{

		//cluster_vector[i]->centroid_accessor()->print_vector();
		//getchar();
		centroid_vector = cluster_vector[i]->kmeans(i);
		if (cluster_vector[i]->is_external()==0)
		{
			cluster_vector[i]->make_external();
			centroid_point = new DataVector(centroid_vector,i);
			cluster_vector[i]->create_external_centroid(centroid_point);
		}
		else
		{
			cluster_vector[i]->change_external_centroid(centroid_vector);
		}
		//cluster_vector[i]->centroid_accessor()->print_vector();
		//getchar();
	}
}

void pam_update(vector <Cluster *> & cluster_vector,string metric)
{
	vector <double > centroid_vector;
	DataVector * centroid_point;
	DataVector * current_centroid;
	double minimum_distance;
	double distance;
	DataVector * new_centroid; 
	for(unsigned int i=0;i<cluster_vector.size();i++)  //vector with centroids for compatibility reasons
	{
		double minimum_distance;
		current_centroid = cluster_vector[i]->centroid_accessor();
		//cout << "old centroid number " << i << " is " <<  cluster_vector[i]->centroid_accessor()->name_accessor();
		minimum_distance = numeric_limits<double>::max();
		if(cluster_vector[i]->is_updated())
		{
			for (auto point_a : cluster_vector[i]->content_accessor() )
			{
				distance = 0.0;
				for (auto point_b : cluster_vector[i]->content_accessor() )	
				{
			    	distance+=vectors_distance(metric,point_a->point_accessor(),point_b->point_accessor());
				}
				if(distance< minimum_distance)
				{
					minimum_distance = distance;
					new_centroid = point_a; 
					cluster_vector[i]->create_external_centroid(new_centroid);
				}	
			}
			//cout << "new centroid number " << i << " is " <<  cluster_vector[i]->centroid_accessor()->name_accessor();
			//cluster_vector[i]->centroid_accessor()->print_vector();
			//getchar();
		}
	}
	return;
}

void silhouette_evaluation(vector <DataVector *> & dataset_vector,vector <Cluster *> & cluster_vector,string metric)
{
	double silhouette;
	int min_cluster;
	int neighbour_cluster;
	double final_silhouette; 
	double distance_a;
	double distance_b;
	double mean_silhouette;
	mean_silhouette = 0.0;
	int counter = 0;
	cout << "in silhouette" << endl;
	for (unsigned int i=0;i<dataset_vector.size();i++)  
	{		
		min_cluster = dataset_vector[i]->cluster_number_accessor().first;
		for (auto point_a : cluster_vector[min_cluster]->content_accessor() )
		{		
			distance_a += point_a->cluster_number_accessor().second;
		}
		distance_a = distance_a/cluster_vector[min_cluster]->content_accessor().size();
		neighbour_cluster = dataset_vector[i]->neighbour_cluster_accessor().first;
		if (neighbour_cluster == min_cluster )
			counter ++;
		if (neighbour_cluster == -1)
			neighbour_cluster = min_cluster;
		for (auto point_a : cluster_vector[neighbour_cluster]->content_accessor() )
		{

			distance_b += point_a->neighbour_cluster_accessor().second;
		}
		distance_b = distance_b/cluster_vector[neighbour_cluster]->content_accessor().size();
		final_silhouette = (distance_b - distance_a)/max(distance_b,distance_a);
		mean_silhouette+=final_silhouette;
    	//cout <<dataset_vector[i]->name_accessor()<< " distance_a " <<distance_a <<"distance_b " << distance_b<< " "<< final_distance << endl;
	}
	mean_silhouette = mean_silhouette/dataset_vector.size();
	cout << counter << " " << mean_silhouette<< endl;
}

int update_cluster_vector(DataVector * v,double distance, vector <Cluster *> &cluster_vector,int cluster_num )
{

	int old_cluster=-1;
	if(v->cluster_number_accessor().first!=-1)
	{
		old_cluster = v->cluster_number_accessor().first;
	}
	if(is_nearest(distance, v ,cluster_num))  
	{
		if (old_cluster!=-1)
		{
			cluster_vector[old_cluster]->remove_from_cluster(v);
		}
		cluster_vector[cluster_num]->add_to_cluster(v); //add point to cluster
		if (old_cluster != cluster_num)  //check if datapoint has changed cluster and affects a different cluster
		{
			if(old_cluster!=-1)
				cluster_vector[old_cluster]->set_update(1);
			cluster_vector[cluster_num]->set_update(1);
		}	
		return 1; 

	}	
	return 0; 
}

void lsh_assignment(int L,int k,HashTable * hashtables_vector,vector <Cluster *> & cluster_vector,string metric,vector <DataVector *> & dataset_vector)
{
	map <DataVector *,double> true_neighbour;
	vector <DataVector *> centroid_vector;
	vector <double> silhouette_vector; 
	double distance;
	int points_has_changed;
	set <DataVector *> unassigned_points;
	double centroid_distance;
	double radius = numeric_limits<double>::max();
	for(unsigned int i=0;i<cluster_vector.size();i++)  //initialize vector with centroids for compatibility reasons
	{
		centroid_vector.push_back(cluster_vector[i]->centroid_accessor());
	}
	int counter =0;
	double new_objective_distance = 0.0;
	double previous_objective_distance = 1.0;
	do
	{	
		if (counter !=0)
		{
			previous_objective_distance = new_objective_distance;
		}
		for (unsigned int i = 0;i<cluster_vector.size();i++)
		{
			cluster_vector[i]->set_update(0);
		}
		new_objective_distance  = 0.0;
		///////////////////////////////////

		for(unsigned int x=0;x<centroid_vector.size();x++) //find radius
		{
			for(unsigned int y=0;y<centroid_vector.size();y++)
			{
				centroid_distance=vectors_distance(metric,centroid_vector[x]->point_accessor(),centroid_vector[y]->point_accessor());
				if(radius> centroid_distance && x!=y)
				{
					radius = centroid_distance;
				}
			}
		}
		radius/=2;
		do
		{
			points_has_changed =0; 
			for(unsigned int cluster_num=0;cluster_num<centroid_vector.size();cluster_num++) //range search for each centroid
			{
				for (int i=0;i<L;i++)
				{
			
					string key= centroid_vector[cluster_num]->key_accessor(i,k);
					for (auto v : hashtables_vector[i][key])
					{
						distance=vectors_distance(metric,centroid_vector[cluster_num]->point_accessor(),v->point_accessor());
						if(distance<radius)
						{
							if(update_cluster_vector(v,distance,cluster_vector,cluster_num ))
							{	
								points_has_changed++; 
								new_objective_distance +=distance * distance;
							}
						}
							
					}
				}
			}
			radius*=2;
		}while(points_has_changed>0);  //there is no need to increase the radius because each point in centroid's bucket is assigned
		
		for(unsigned i=0;i<dataset_vector.size();i++)
		{
			if(dataset_vector[i]->cluster_number_accessor().first == -1) //for unassigned points
			{
				for(unsigned cluster_num=0;cluster_num<cluster_vector.size();cluster_num++)
				{
					distance=vectors_distance(metric,centroid_vector[cluster_num]->point_accessor(),dataset_vector[i]->point_accessor());
					update_cluster_vector(dataset_vector[i],distance,cluster_vector,cluster_num );
					new_objective_distance +=distance * distance;
				
				}
			}
		}
		cout << "objective_distance " << new_objective_distance << "/"<< previous_objective_distance<< endl;	
		lloyds_update(cluster_vector);
		//pam_update(cluster_vector,metric);
		for(unsigned int i=0;i<cluster_vector.size();i++)  //initialize vector with centroids for compatibility reasons
		{
			centroid_vector[i] = cluster_vector[i]->centroid_accessor();
		}
		counter++;
	}while(new_objective_distance!=	previous_objective_distance);
}
